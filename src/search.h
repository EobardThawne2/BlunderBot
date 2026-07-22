#pragma once
#include "board.h"
#include "move.h"
#include <chrono>

#include <atomic>

extern std::atomic<bool> global_stop;

struct SearchInfo {
    int nodes;
    long long start_time;
    long long time_limit;

    Move killer_moves[64][2];     // [depth][slot]
    int history_table[2][64][64]; // [color][from][to]
    Move countermoves[64][64];    // [prev_move.from()][prev_move.to()]
};

Move search(Board board, int depth_limit, long long time_limit_ms);
