#include "tt.h"
#include <iostream>

TranspositionTable TT;

// Helper functions for packing/unpacking TT data
static uint64_t pack_data(uint64_t key, int depth, int score, int flag, Move best_move) {
    uint64_t k = (key >> 48) & 0xFFFFULL;
    uint64_t d = ((uint64_t)depth & 0xFFULL);
    uint64_t f = ((uint64_t)flag & 0x3ULL);
    uint64_t s = ((uint64_t)(uint16_t)(int16_t)score & 0xFFFFULL);
    uint64_t m = ((uint64_t)best_move.move & 0xFFFFULL);
    return (k << 48) | (d << 40) | (f << 32) | (s << 16) | m;
}

static void unpack_data(uint64_t data, uint64_t &key_sig, int &depth, int &score, int &flag, Move &best_move) {
    key_sig = (data >> 48) & 0xFFFFULL;
    depth = (data >> 40) & 0xFFULL;
    flag = (data >> 32) & 0x3ULL;
    score = (int)(int16_t)((data >> 16) & 0xFFFFULL);
    best_move = Move(data & 0xFFFFULL);
}

TranspositionTable::TranspositionTable() {
    table = nullptr;
    resize(32); // Default to 32MB
}

TranspositionTable::~TranspositionTable() {
    delete[] table;
}

void TranspositionTable::resize(int mb) {
    int bytes = mb * 1024 * 1024;
    int num_entries = bytes / sizeof(TTEntry);

    int power_of_2_entries = 1;
    while (power_of_2_entries * 2 <= num_entries) { power_of_2_entries *= 2; }

    if (table != nullptr) { delete[] table; }
    table = new TTEntry[power_of_2_entries];
    size_mask = power_of_2_entries - 1;
    clear();
}

void TranspositionTable::clear() {
    for (uint64_t i = 0; i <= size_mask; i++) {
        table[i].data.store(0, std::memory_order_relaxed);
    }
}

void TranspositionTable::store(uint64_t key, int depth, int score, int flag, Move best_move) {
    int index = key & size_mask;
    uint64_t packed = pack_data(key, depth, score, flag, best_move);
    table[index].data.store(packed, std::memory_order_relaxed);
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int &score, Move &best_move) {
    int index = key & size_mask;
    uint64_t stored_data = table[index].data.load(std::memory_order_relaxed);

    if (stored_data == 0) return false;

    uint64_t key_sig;
    int entry_depth, entry_flag;
    unpack_data(stored_data, key_sig, entry_depth, score, entry_flag, best_move);

    if (key_sig == ((key >> 48) & 0xFFFFULL)) {
        if (entry_depth >= depth) {
            if (entry_flag == TT_EXACT) { return true; }
            if (entry_flag == TT_ALPHA && score <= alpha) {
                score = alpha;
                return true;
            }
            if (entry_flag == TT_BETA && score >= beta) {
                score = beta;
                return true;
            }
        }
    }
    return false;
}
