#pragma once
#include <cstdint>
#include <vector>
#include <atomic>
#include "move.h"

// Flags for transposition table
const int TT_EXACT = 0;
const int TT_ALPHA = 1;
const int TT_BETA = 2;

struct TTEntry {
    std::atomic<uint64_t> key;
    std::atomic<uint64_t> data;
};

class TranspositionTable {
  public:
    TranspositionTable();
    ~TranspositionTable();
    void resize(int mb);
    void clear();
    void store(uint64_t key, int depth, int score, int flag, Move best_move);
    bool probe(uint64_t key, int depth, int alpha, int beta, int &score, Move &best_move);

  private:
    TTEntry *table;
    uint64_t size_mask;

    // Disable copy for table
    TranspositionTable(const TranspositionTable &) = delete;
    TranspositionTable &operator=(const TranspositionTable &) = delete;
};

extern TranspositionTable TT;
