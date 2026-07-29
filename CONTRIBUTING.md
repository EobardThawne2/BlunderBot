# Contributing to BlunderBot

Thank you for your interest in contributing to **BlunderBot**! We welcome contributions from chess engine developers, C++ programmers, and researchers interested in search algorithms, neural network evaluation (NNUE), and chess programming performance optimization.

This document outlines the guidelines and workflow for contributing to the repository.

---

## 1. Prerequisites and Development Environment

Before building and testing your changes, ensure your environment meets the following requirements:

- **C++ Compiler**: C++20 compliant compiler (`GCC 11+`, `Clang 13+`, or `MSVC 2022+`).
- **Build System**: `CMake 3.16+`.
- **Formatting**: `clang-format` (a `.clang-format` configuration is provided in the repository).
- **Match Runner (Optional for local testing)**: `c-chess-cli` or `cutechess-cli` for running local Elo testing.

### Building locally

```bash
# Clone the repository
git clone https://github.com/EobardThawne2/BlunderBot.git
cd BlunderBot

# Configure and build in Release mode
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## 2. Code Style and Guidelines

To maintain clean and readable code across the codebase, please adhere to the following rules:

- **Formatting**: Run `clang-format -i src/*.cpp src/*.h` on modified C++ source files prior to opening a pull request.
- **Modern C++**: Use modern C++20 standard practices (`std::atomic`, `constexpr`, zero-branching bitwise intrinsics where applicable).
- **Comments and Documentation**: Avoid cluttering code with obvious comments, but document non-trivial mathematical formulas, bitboard operations, or complex search heuristics.
- **No External Engine Binaries**: Do not commit compiled executables (`.exe`, ELF binaries) or large temporary datasets to the repository.

---

## 3. Pull Request Guidelines & SPRT Testing

BlunderBot uses strict statistical testing to ensure that engine modifications do not cause functional or performance regressions.

### Automated CI Regression Testing
When you open a Pull Request, our GitHub Actions workflow automatically runs a **Sequential Probability Ratio Test (SPRT)** against the `main` branch.

- **Match Runner**: `c-chess-cli`
- **SPRT Bounds**: `elo0 = -10`, `elo1 = 0` (`alpha = 0.05`, `beta = 0.05`)
- **Ablation Testing**: The workflow tests feature ablations (SEE, Singular Extensions, Countermoves, ProbCut) symmetrically.
- **Passing Criteria**: A PR must pass the SPRT test (or demonstrate neutral Elo without performance regression) to be merged.

### Submitting a PR
1. **Fork the repository** and create a feature branch (`git checkout -b feature/my-feature`).
2. **Keep PRs focused**: Make small, self-contained changes (e.g. a single search tweak or bug fix) rather than large multi-feature overhauls.
3. **Write descriptive commit messages**: Summarize what changed and the rationale behind it.
4. **Push and open a PR**: Target the `main` branch of `EobardThawne2/BlunderBot`.

---

## 4. Reporting Issues & Feature Requests

If you encounter bugs or have feature suggestions:
- Check existing GitHub Issues to see if the problem has already been reported.
- When opening an issue, provide:
  - OS and compiler version.
  - Precise steps to reproduce (or UCI log / FEN string if a crash or illegal move occurred).
  - Expected behavior vs. actual behavior.

Thank you for helping make BlunderBot stronger! ♟️
