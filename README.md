# BlunderBot Chess Engine

A lightweight, high-performance C++ chess engine built from scratch.

## Features

- **Core Engine:** Written purely in C++20 for maximum performance.
- **Board Representation:** 64-bit Bitboards for lightning-fast move generation.
- **Move Generation:** Magic Bitboards for extremely efficient sliding piece moves (Rooks and Bishops).
- **Search Algorithm:** Negamax with Alpha-Beta Pruning.
- **Transposition Table:** Zobrist Hashing to cache and reuse evaluated positions.
- **Evaluation Engine:** 
  - NNUE (Efficiently Updatable Neural Networks) integration for state-of-the-art position evaluation.
  - Tapered Piece-Square Tables (PSTs) fallback for middlegame and endgame.
- **Endgame Tablebases:** Syzygy Tablebase support for perfect endgame play (up to 5 pieces).
- **Protocols:** Full support for the Universal Chess Interface (UCI) protocol, compatible with major chess GUIs (Arena, Cute Chess, etc.).
- **User Interfaces:**
  - Built-in Terminal User Interface (TUI) with an interactive colored board.
  - Real-time Web UI via a FastAPI WebSocket server.

## Project Structure

- `src/` - Core C++ Engine source code.
- `server/` - Python FastAPI server that acts as a bridge between the Web UI and the engine.
- `ui/` - Frontend HTML/JS/CSS for playing against the engine directly in your web browser.
- `syzygy/` - Contains Syzygy endgame tablebase files.
- `Dockerfile` - For easy containerized deployment of the web server.

## Requirements

- **C++ Compiler:** GCC, Clang, or MSVC supporting C++20.
- **CMake:** Version 3.10 or higher.
- **Python:** 3.10+ (optional, for running the Web UI server).
- **Docker:** (optional, for containerized deployments).

## Build Instructions

### Local C++ Build

This engine uses `CMake` for building the native executable.

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The resulting executable (`BlunderBot` or `BlunderBot.exe`) will be located in the `build` directory along with a copy of the required NNUE weights file.

### Docker Build (Web Server)

If you want to run the web interface without installing dependencies locally:

```bash
docker build -t blunderbot-web .
docker run -p 8000:8000 blunderbot-web
```
Navigate to `http://localhost:8000` to play!

## Running the Engine

### Terminal User Interface (TUI)
You can play interactively in your terminal by passing the `tui` flag (ensure you run it from a directory where it can find the `.nnue` file, typically the `build` folder):
```bash
cd build
./BlunderBot tui
```
*(On Windows: `.\BlunderBot.exe tui`)*

### UCI Mode
Run the engine without arguments to start it in UCI mode. This allows you to connect it to any standard Chess GUI:
```bash
cd build
./BlunderBot
```

### Web UI (Local Python)
If you built the engine locally and have Python installed, you can start the WebSocket server to play in your browser:
```bash
pip install -r requirements.txt
uvicorn server.server:app --host 127.0.0.1 --port 8000
```
Then visit `http://127.0.0.1:8000`.

## Tablebases
The engine supports Syzygy tablebase files (`.rtbw` / `.rtbz`) to play perfectly in endgames. You can download missing ones using the provided `download_syzygy.py` script and ensure they reside in the `syzygy/` directory.
