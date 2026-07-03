# BlunderBot Chess Engine

A lightweight, purely C++ chess engine built from scratch.

## Features Currently Implemented
- **Board Representation:** Bitboards for lightning-fast move generation.
- **Move Generation:** Magic Bitboards for sliding pieces (Rooks/Bishops).
- **Search Algorithm:** Negamax with Alpha-Beta Pruning.
- **Transposition Table:** Zobrist Hashing for caching evaluated positions.
- **Evaluation:** Tapered Piece-Square Tables (PSTs) for middlegame and endgame.
- **Interface:** Terminal User Interface (TUI) with colored pieces and interactive move input.

## Build Instructions
This engine uses `CMake` for building.

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Running
After building, run the executable with the `tui` flag to play interactively in your terminal:
```bash
./BuildA.exe tui
```
(On Linux/Mac, run `./BuildA tui`)
