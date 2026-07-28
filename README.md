# BlunderBot Chess Engine Architecture

BlunderBot is an advanced, high-performance, Universal Chess Interface (UCI) compliant chess engine written in C++20. Engineered for strict modularity, raw execution speed, and adherence to modern chess programming paradigms, it integrates state-of-the-art heuristic search algorithms and a custom Efficiently Updatable Neural Network (NNUE) evaluation architecture.

To interact with the latest deployment of the BlunderBot engine, visit:
https://blunderbot-l52n.onrender.com/

---

## 1. Core Engine and Board Representation

### 1.1 64-bit Bitboards
The foundational data structure of BlunderBot is the 64-bit unsigned integer (`uint64_t`), representing a bitboard. BlunderBot maintains an array of bitboards for each piece type (Pawn, Knight, Bishop, Rook, Queen, King) and color (White, Black). This allows the engine to perform highly parallelized, vectorized piece operations using bitwise arithmetic and intrinsic CPU instructions:
- `__builtin_popcountll`: Used to count the number of set bits (e.g., evaluating material count or mobility).
- `__builtin_ctzll`: Used to find the index of the Least Significant 1-Bit (LS1B), enabling rapid serialization of bitboards into discrete piece coordinate lists.

Bitwise operations (AND, OR, XOR, NOT, and shifts) provide a zero-branching methodology for evaluating board states. For instance, determining if a square is occupied by a White Pawn simply requires checking if `(board.piece_bb[PAWN] & board.color_bb[WHITE]) & (1ULL << square)` evaluates to a non-zero value.

### 1.2 Move Generation
BlunderBot utilizes a staged, pseudo-legal move generator to maximize node throughput. The generator distinguishes between capturing and non-capturing moves, which is vital for search routines like Quiescence Search where only forcing moves are expanded.
- **Leaper Pieces (Knights, Kings):** Moves are generated using pre-calculated attack masks indexed by the piece's square. The masks are generated recursively during the engine initialization phase (`init_all()`).
- **Pawn Mechanics:** Pawn pushes, double pushes, and attacks (including En Passant) are generated via bitwise shifts, heavily optimizing the most common moves on the board. The engine strictly avoids conditional branching here.
- **Sliding Pieces (Rooks, Bishops, Queens):** BlunderBot leverages **Magic Bitboards**. By hashing the occupancy of the relevant blocker squares (multiplying by a pre-computed "magic number" and right-shifting), sliding piece attacks are resolved via a direct array lookup in O(1) constant time. This strictly eliminates the computationally expensive loop-based ray casting historically used in older engines.
- **Legality Checking:** Moves are generated pseudo-legally (allowing moves that might leave the King in check). Legality is strictly verified immediately before the move is executed in the search tree, preventing the generation overhead for branches that are heavily pruned.

### 1.3 Zobrist Hashing
Position states are tracked using 64-bit Zobrist Hashing. A unique, pseudo-random 64-bit integer is assigned to every possible piece-square combination, castling right, en passant file, and the side-to-move. As pieces move, the hash is incrementally updated via XOR operations. This hash serves as the primary key for the Transposition Table and allows mathematically precise detection of 3-fold repetitions and the 50-move rule.

---

## 2. Search Framework (Alpha-Beta Negamax)

BlunderBot implements an iterative deepening Alpha-Beta Negamax search algorithm, augmented with Principal Variation Search (PVS). PVS assumes that the first move searched is the Principal Variation (the best move) and searches it with a full window. Subsequent moves are searched with a zero-window (null window) to prove they are worse; a full re-search is only triggered if the zero-window search fails high.

### 2.1 The Transposition Table (TT)
The Transposition Table is a highly optimized, lock-free hash map that caches search results. Each TT entry stores:
- **Zobrist Key (Partial):** To verify collisions.
- **Depth:** The search depth at which the position was evaluated.
- **Score:** The static or backed-up evaluation score.
- **Bound Type:** Exact, Lower Bound (Beta cutoff), or Upper Bound (Alpha cutoff).
- **Best Move:** The move that yielded the highest score.

Before expanding a node, BlunderBot probes the TT. If a valid entry exists with a depth greater than or equal to the current required depth, the search immediately returns the cached score, circumventing the entire subtree calculation.

### 2.2 Move Ordering
The efficiency of Alpha-Beta pruning scales exponentially with the quality of move ordering. BlunderBot enforces a strict deterministic ordering schema to force beta-cutoffs as early as possible:
1. **Hash Move:** The best move extracted from the TT. Statistically, this is the most likely move to cause a cutoff.
2. **Winning Captures (SEE >= 0):** Captures and promotions are dynamically sorted using Static Exchange Evaluation (SEE), which simulates the sequence of captures on a target square to determine the net material gain/loss.
3. **Killer Moves:** Up to two non-capturing quiet moves that caused a beta-cutoff in sibling nodes at the exact same ply are prioritized.
4. **Countermoves:** A historical table indexed by `[piece][to_square]` of the opponent's previous move, returning a quiet move that logically counters the threat.
5. **History Moves:** Quiet moves are dynamically sorted by a continuously updated History Table. Moves that cause cutoffs deeper in the search tree increment their historical weight, allowing the engine to mathematically learn which quiet maneuvers are effective in the current position.
6. **Losing Captures (SEE < 0):** Captures that lose material are pushed to the absolute end of the evaluation queue.

### 2.3 Forward Pruning and Reductions
To achieve extreme depth within modern time controls, BlunderBot aggressively prunes unpromising branches:
- **Null Move Pruning (NMP):** Operates on the assumption that passing a turn (a null move) is the worst possible action. If the evaluation after a null move still exceeds beta, the opponent's threat is deemed insignificant, and the branch is pruned.
- **Late Move Reductions (LMR):** Quiet moves ordered late in the sequence are assumed to be tactically flawed. Their search depth is logarithmically reduced based on the current nominal depth and their index in the move list. If the reduced search exceeds alpha, the move is re-searched at full depth.
- **Futility Pruning:** If a node is near the horizon (Depth <= 2), and the static evaluation plus a mathematical margin falls significantly below alpha, the engine assumes recovery is impossible and prunes the node.
- **Reverse Futility Pruning (Static NMP):** Evaluated at the pre-node expansion phase. If the static evaluation exceeds beta by a massive margin, the node returns immediately without generating moves.
- **ProbCut:** Performs a highly aggressive, shallow search (typically depth - 4) with a significant beta margin. If this shallow search causes a cutoff, the main search is bypassed entirely.
- **Delta Pruning:** Used exclusively within the Quiescence Search algorithm. If the current static evaluation plus a significant material margin (e.g., the value of a Queen) remains beneath the alpha bound, the branch is terminated.

### 2.4 Search Extensions
- **Singular Extensions:** When the Hash Move score exceeds the score of all alternative moves by a significant, pre-defined margin, it is classified as "singular" (forced). BlunderBot extends the search depth of singular moves by 1 ply, ensuring absolute tactical accuracy in forcing lines and mitigating the horizon effect.
- **Quiescence Search (Q-Search):** Triggered when the nominal search depth reaches zero. Q-Search recursively generates and evaluates all forcing tactical sequences (captures and Queen promotions) until a statically "quiet" position is achieved. This ensures that the engine does not erroneously evaluate a position midway through a piece exchange.

---

## 3. Evaluation and Neural Architecture

BlunderBot abandons traditional Hand-Crafted Evaluation (HCE) entirely, relying strictly on an Efficiently Updatable Neural Network (NNUE) for static positional assessment. NNUE evaluates positions intrinsically, providing superior positional understanding regarding king safety, pawn structure, and piece coordination.

### 3.1 Network Topology (HalfKP)
The engine utilizes a custom `Blunderbot.nnue` weights file processed natively on the CPU via the `nnue-probe` library. 
- **Input Layer:** The architecture employs a Half-King-Piece (HalfKP) mapping. The input vector consists of 41,024 discrete features, representing the relationship between the active King's square and every other piece on the board. The model considers the position relative to the side to move.
- **Hidden Layers:** The network processes these features through highly optimized, densely connected hidden layers using clipped ReLU activations, dynamically calculating non-linear positional heuristics that are mathematically impossible to express in HCE.
- **Incremental Updates:** Rather than recalculating the entire 41,024-feature input array from scratch at every leaf node, BlunderBot incrementally updates the network's accumulator during `make_move` and `unmake_move` operations. A single piece movement only updates the specific neural weights associated with the "from" and "to" squares, allowing NNUE to rival the speed of primitive HCE calculations. 
- **Scale Invariance:** NNUE evaluations inherently encapsulate positional heuristics across all game phases (Opening, Middlegame, Endgame). Unlike legacy HCE models that require artificial scaling based on non-pawn material counts, NNUE evaluates endgame theoretical probabilities directly, preventing the structural distortions that arise from arbitrary programmatic phase-shifting.

---

## 4. Opening Book and PolyGlot Integration

BlunderBot natively supports the binary PolyGlot `.bin` opening book format. 
- The repository includes a standalone `make_book` C++ executable that mathematically compiles raw PGN strings into a highly compressed, Zobrist-indexed binary format during the CMake build sequence.
- During the root search phase, if the current Zobrist hash matches an entry in the compiled `blunderbot_book.bin`, the engine immediately plays the pre-calculated theoretical move, completely bypassing the Negamax search tree and conserving computational resources for out-of-book middlegame positions.
- The engine uses deterministic hashing to traverse the Polyglot book, ensuring the theoretical line is selected correctly.

---

## 5. Continuous Integration (CI/CD) and Automated Testing

Strict statistical rigor governs the BlunderBot codebase. A fully automated GitHub Actions CI/CD pipeline validates every commit to the repository. The continuous testing protocol ensures no regressions are merged into the main branch.

### 5.1 Automated SPRT Regression
- Every pull request initiates an ablation test matrix pitting the modified `pr-branch` executable against the stable `main` baseline.
- `c-chess-cli` acts as the deterministic match runner, enforcing strict time controls and zero-variance concurrency.
- Matches are mathematically evaluated using the Sequential Probability Ratio Test (SPRT). The testing parameters are defined as `elo0=-10` and `elo1=0` with error bounds `alpha=0.05` and `beta=0.05`. 
- For code changes to be merged, they must mathematically demonstrate equal or greater Elo strength (i.e., failing the null hypothesis that the new engine is worse) against the identical baseline constraint. Heuristic ablation tests enforce symmetry by selectively disabling UCI configuration parameters across both instances. This isolating methodology prevents regressions across disparate features like SEE, Singular Extensions, Countermoves, and ProbCut.
