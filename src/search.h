#pragma once
#include "board.h"
#include "move.h"
#include <chrono>

struct SearchInfo {
    int nodes;
    bool stopped;
    long long start_time;
    long long time_limit;

    Move killer_moves[64][2]; // [depth][slot]
    int history_table[2][64][64]; // [color][from][to]
};

extern SearchInfo info;

void clear_heuristics();
Move search(Board& board, int depth_limit, long long time_limit_ms);
