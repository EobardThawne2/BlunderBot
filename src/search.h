#pragma once
#include "board.h"
#include "move.h"
#include <chrono>

#include <atomic>

extern std::atomic<bool> global_stop;

extern bool use_see;
extern bool use_singular_extensions;
extern bool use_countermove;
extern bool use_probcut;

struct SearchInfo {
    int nodes;
    long long start_time;
    long long time_limit;

    Move killer_moves[64][2];     // [depth][slot]
    int history_table[2][64][64]; // [color][from][to]
    Move countermoves[64][64];    // [prev_move.from()][prev_move.to()]
};

Move search(const Board &board, int depth_limit, long long time_limit_ms);
