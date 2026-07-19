# BlunderBot Chess Engine

BlunderBot is a modern, high-performance chess engine developed in C++20. Designed with a focus on modularity, raw execution speed, and adherence to modern chess programming paradigms, BlunderBot utilizes state-of-the-art evaluation techniques and highly optimized search algorithms.

## Features & Architecture

### Core Engine
- **Language:** Written entirely in C++20 for maximum performance.
- **Board Representation:** 64-bit Bitboards for rapid piece manipulation and state evaluation.
- **Hashing:** Zobrist Hashing for efficient position tracking and Transposition Table (TT) lookups.
- **Move Generation:** Utilizes Magic Bitboards to instantly resolve sliding piece attacks (Rooks/Bishops), entirely eliminating loop-based ray tracing overhead.

### Search Algorithm (Alpha-Beta Negamax)
BlunderBot implements a highly tuned Alpha-Beta Negamax search framework with the following enhancements:
- **Transposition Table:** Caches bounds and best moves of previously evaluated positions to prevent redundant calculations.
- **Move Ordering:** Highly optimized move ordering utilizing Hash Moves, MVV-LVA (Most Valuable Victim - Least Valuable Attacker) for captures, Killer Heuristic, and History Heuristic.
- **Pruning & Reductions:**
  - **Null Move Pruning (NMP):** Aggressively prunes branches where the side-to-move is overwhelmingly winning.
  - **Late Move Reductions (LMR):** Dynamically reduces the search depth for historically poor moves, drastically increasing search speed.
  - **Futility & Reverse Futility Pruning:** Prunes near-leaf nodes based on static evaluation bounds.
- **Quiescence Search:** Extends the search horizon for forcing moves (captures) to prevent the horizon effect.
- **Aspiration Windows:** Constrains the root search bounds to achieve massive beta-cutoffs.
- **Lazy SMP:** Supports multithreaded evaluation by spawning asynchronous worker threads that share a global Transposition Table.

### Evaluation Function
- **NNUE Integration:** BlunderBot relies entirely on Efficiently Updatable Neural Networks (NNUE) for static evaluation. 
- **Library:** Powered by the `nnue-probe` library, evaluating a half-kp architecture network (`nn-62ef826d1a6d.nnue`) natively on the CPU for extremely precise positional understanding.

### Opening Book
- **PolyGlot Support:** Native parsing of standard `.bin` PolyGlot opening books.
- **Custom Compiler:** Includes a standalone `make_book` executable that compiles raw PGN/TXT opening lines into optimized binary formats during the build process.

### Continuous Integration (CI/CD)
- **Automated SPRT Testing:** Fully automated GitHub Actions pipeline (`elo-test.yml`) that validates every commit.
- **Regression Testing:** Compiles the latest code and pits it against the stable `main` branch using `c-chess-cli` and Sequential Probability Ratio Testing (SPRT) to mathematically prove Elo gains.

---

## Interfaces & Protocols

- **UCI Protocol:** Comprehensive support for the Universal Chess Interface (UCI) protocol, ensuring seamless integration with standard chess GUIs (e.g., Arena, Cute Chess).
- **Command-Line Interface:** Native Terminal User Interface (TUI) for direct engine interaction.
- **Web UI:** Asynchronous Python/FastAPI WebSocket server driving a browser-based frontend (`/server` and `/ui`).

---

## Build Instructions

### System Requirements
- **C++ Compiler:** GCC, Clang, or MSVC with C++20 support.
- **CMake:** Version 3.10 or higher.
- **Python:** Version 3.10+ (required only for the Web UI server).

### Local C++ Compilation
The native executable is built using CMake. This process will also automatically compile the opening book.

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The resulting executable (`BlunderBot` or `BlunderBot.exe`) will be generated within the `build` directory alongside the required `.nnue` weights file and compiled `.bin` opening book.

---

## Usage Guide

### UCI Mode (GUI Integration)
Executing the engine without arguments initializes it in UCI mode. Point your preferred Chess GUI (like Arena or Cute Chess) to this executable.
*Note: Ensure the `.nnue` weights file is in the same directory as the executable.*

```bash
cd build
./BlunderBot
```

### Terminal User Interface (TUI)
To play against the engine directly in your terminal:

```bash
cd build
./BlunderBot tui
```
*(Windows: `.\BlunderBot.exe tui`)*

### Local Web UI (Python)
If the engine is compiled locally and Python is installed, you can launch the WebSocket server to host the browser interface:

```bash
pip install -r requirements.txt
uvicorn server.server:app --host 127.0.0.1 --port 8000
```
Access the interface at `http://127.0.0.1:8000`.

### Docker Container (Web Server)
A pre-built Docker image is available on the GitHub Container Registry.

```bash
docker pull ghcr.io/eobardthawne2/blunderbot:main
docker run -p 8000:8000 ghcr.io/eobardthawne2/blunderbot:main
```
The web interface will be accessible at `http://localhost:8000`.
