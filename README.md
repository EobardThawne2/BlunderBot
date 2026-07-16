# BlunderBot Chess Engine

BlunderBot is a lightweight, high-performance C++ chess engine designed for efficient evaluation and search.

## Features

- **Core Engine:** Developed in C++20 for optimal execution speed.
- **Board Representation:** Utilizes 64-bit bitboards to optimize move generation overhead.
- **Move Generation:** Implements Magic Bitboards for highly efficient sliding piece (Rook and Bishop) move calculations.
- **Search Algorithm:** Employs Negamax combined with Alpha-Beta Pruning, enhanced with a Transposition Table (Zobrist Hashing) to cache evaluated positions.
- **Evaluation Engine:**
  - Integrated Efficiently Updatable Neural Networks (NNUE) for state-of-the-art static position evaluation.
  - Tapered Piece-Square Tables (PSTs) provided as a fallback heuristic.
- **Protocols:** Comprehensive support for the Universal Chess Interface (UCI) protocol, ensuring compatibility with standard chess graphical user interfaces (e.g., Arena, Cute Chess).
- **User Interfaces:**
  - Command-line Terminal User Interface (TUI) for direct interaction.
  - Asynchronous Web UI facilitated via a FastAPI WebSocket server.

## Project Architecture

- `src/` - Core C++ Engine source code.
- `server/` - Python FastAPI server functioning as the middleware between the Web UI and the engine.
- `ui/` - Frontend HTML/JS/CSS assets for the browser-based interface.
- `Dockerfile` - Configuration for containerized deployment of the web server.

## System Requirements

- **C++ Compiler:** GCC, Clang, or MSVC with C++20 support.
- **CMake:** Version 3.10 or higher.
- **Python:** Version 3.10 or higher (required only for the Web UI server).
- **Docker:** (Optional, for containerized deployments).

## Pre-built Binaries

Pre-compiled binaries for major operating systems are provided via GitHub Releases.

1. Navigate to the [Releases page](https://github.com/EobardThawne2/BlunderBot/releases/latest).
2. Download the appropriate `.zip` archive for your target platform (Windows, Ubuntu, or macOS).
3. Extract the archive contents.
4. Execute the engine via the command line as detailed in the Usage section below.

## Build Instructions

### Local C++ Compilation

The native executable is built using CMake.

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The resulting executable (`BlunderBot` or `BlunderBot.exe`) will be generated within the `build` directory alongside the required NNUE weights file.

### Docker Container (Web Server)

A pre-built Docker image is available on the GitHub Container Registry. You can pull and run the latest version using the following commands:

```bash
docker pull ghcr.io/eobardthawne2/blunderbot:main
docker run -p 8000:8000 ghcr.io/eobardthawne2/blunderbot:main
```

The web interface will be accessible at `http://localhost:8000`.

Alternatively, to build the container locally from the source:

```bash
docker build -t blunderbot-web .
docker run -p 8000:8000 blunderbot-web
```

## Usage

### Terminal User Interface (TUI)

The engine can be executed interactively within the terminal. Ensure the command is run from a directory containing the `.nnue` weights file (typically the `build` directory).

```bash
cd build
./BlunderBot tui
```
*(Windows: `.\BlunderBot.exe tui`)*

### UCI Mode

Executing the engine without arguments initializes it in UCI mode, which is the standard operation mode for connecting to external Chess GUIs.

```bash
cd build
./BlunderBot
```

### Local Web UI (Python)

If the engine is compiled locally and Python is installed, the WebSocket server can be started to host the browser interface:

```bash
pip install -r requirements.txt
uvicorn server.server:app --host 127.0.0.1 --port 8000
```

The interface will then be available at `http://127.0.0.1:8000`.
