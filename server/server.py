import asyncio
import os
import subprocess
import threading
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
import uvicorn


app = FastAPI()

# Store connected websockets
clients = set()

engine_path = os.path.join(
    os.path.dirname(os.path.dirname(__file__)),
    'build',
    'Release' if os.name == 'nt' else '',
    'BlunderBot.exe' if os.name == 'nt' else 'BlunderBot'
)
engine_process = None
engine_output_queue = asyncio.Queue()
loop = None


def read_engine_output(process, event_loop, queue):
    while True:
        line = process.stdout.readline()
        if not line:
            break
        line_str = line.decode('utf-8', errors='ignore').strip()
        if line_str:
            asyncio.run_coroutine_threadsafe(queue.put(line_str), event_loop)


async def broadcast_engine_output():
    while True:
        line_str = await engine_output_queue.get()
        dead_clients = set()
        for client in clients:
            try:
                await client.send_text(line_str)
            except Exception:
                dead_clients.add(client)

        for client in dead_clients:
            clients.remove(client)


@app.on_event("startup")
async def startup_event():
    global engine_process, loop
    loop = asyncio.get_running_loop()

    build_dir = os.path.dirname(engine_path)
    # Use standard subprocess.Popen instead of asyncio for compatibility
    engine_process = subprocess.Popen(
        engine_path,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=build_dir,
        bufsize=1  # Line buffered
    )

    # Start reader thread
    threading.Thread(
        target=read_engine_output,
        args=(engine_process, loop, engine_output_queue),
        daemon=True
    ).start()

    # Start broadcast task
    asyncio.create_task(broadcast_engine_output())

    # Initialize UCI
    engine_process.stdin.write(b"uci\n")
    engine_process.stdin.write(b"isready\n")
    engine_process.stdin.flush()


@app.on_event("shutdown")
async def shutdown_event():
    if engine_process:
        engine_process.terminate()


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    try:
        while True:
            data = await websocket.receive_text()
            if engine_process:
                # Send the received command directly to the engine
                engine_process.stdin.write(f"{data}\n".encode('utf-8'))
                engine_process.stdin.flush()
    except WebSocketDisconnect:
        clients.remove(websocket)


# Mount static files at root
ui_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'ui')
app.mount(
    "/",
    StaticFiles(directory=ui_dir, html=True),
    name="ui"
)

if __name__ == "__main__":
    uvicorn.run("server:app", host="127.0.0.1", port=8000, reload=True)
