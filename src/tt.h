#pragma once
#include <cstdint>
#include <vector>
#include "move.h"

enum TTFlag {
    TT_EXACT,
    TT_ALPHA,
    TT_BETA
};

struct TTEntry {
    uint64_t key;
    int score;
    int depth;
    int flag;
    Move best_move;
};

class TranspositionTable {
public:
    std::vector<TTEntry> entries;
    uint64_t size_mask;

    TranspositionTable();
    void resize(int mb);
    void clear();
    
    void store(uint64_t key, int depth, int score, int flag, Move best_move);
    bool probe(uint64_t key, int depth, int alpha, int beta, int& score, Move& best_move);
};

extern TranspositionTable TT;
