import subprocess
import time
import os
import sys

def main():
    engine_path = os.path.join("build", "BlunderBot")
    if not os.path.exists(engine_path):
        engine_path = engine_path + ".exe"
    
    print(f"Testing engine: {engine_path}")
    process = subprocess.Popen(
        [engine_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    def send(cmd):
        print(f"> {cmd}")
        process.stdin.write(cmd + "\n")
        process.stdin.flush()

    def wait_for(expected, timeout=5):
        start = time.time()
        while time.time() - start < timeout:
            line = process.stdout.readline()
            if line:
                line = line.strip()
                print(f"< {line}")
                if expected in line:
                    return True
        return False

    # Smoke test sequence
    send("uci")
    if not wait_for("uciok"):
        print("FAILED: Did not receive uciok")
        sys.exit(1)

    send("isready")
    if not wait_for("readyok"):
        print("FAILED: Did not receive readyok")
        sys.exit(1)

    send("position startpos")
    send("go depth 3")
    if not wait_for("bestmove", timeout=10):
        print("FAILED: Did not receive bestmove")
        sys.exit(1)

    send("quit")
    process.wait()
    print("Smoke test passed successfully!")

if __name__ == "__main__":
    main()
