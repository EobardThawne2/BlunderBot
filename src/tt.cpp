#include "tt.h"
#include <iostream>

TranspositionTable TT;

// Helper functions for packing/unpacking TT data
static uint64_t pack_data(int depth, int score, int flag, Move best_move) {
    uint64_t d = ((uint64_t)depth & 0xFFULL);
    uint64_t s = ((uint64_t)(uint16_t)(int16_t)score & 0xFFFFULL);
    uint64_t f = ((uint64_t)flag & 0x3ULL);
    uint64_t m = ((uint64_t)best_move.move & 0xFFFFULL);
    return (d << 48) | (f << 40) | (s << 16) | m;
}

static void unpack_data(uint64_t data, int& depth, int& score, int& flag, Move& best_move) {
    depth = (data >> 48) & 0xFFULL;
    flag = (data >> 40) & 0x3ULL;
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
    while (power_of_2_entries * 2 <= num_entries) {
        power_of_2_entries *= 2;
    }
    
    if (table != nullptr) {
        delete[] table;
    }
    table = new TTEntry[power_of_2_entries];
    size_mask = power_of_2_entries - 1;
    clear();
}

void TranspositionTable::clear() {
    for (uint64_t i = 0; i <= size_mask; i++) {
        table[i].key.store(0, std::memory_order_relaxed);
        table[i].data.store(0, std::memory_order_relaxed);
    }
}

void TranspositionTable::store(uint64_t key, int depth, int score, int flag, Move best_move) {
    int index = key & size_mask;
    uint64_t packed = pack_data(depth, score, flag, best_move);
    uint64_t xor_data = packed ^ key;
    
    table[index].data.store(xor_data, std::memory_order_relaxed);
    table[index].key.store(key, std::memory_order_release);
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int& score, Move& best_move) {
    int index = key & size_mask;
    
    uint64_t stored_key = table[index].key.load(std::memory_order_acquire);
    uint64_t stored_data = table[index].data.load(std::memory_order_relaxed);
    
    if (stored_key == key) {
        uint64_t unpacked = stored_data ^ key;
        
        int entry_depth, entry_flag;
        unpack_data(unpacked, entry_depth, score, entry_flag, best_move);
        
        // Basic sanity check against torn reads
        if (entry_flag < TT_EXACT || entry_flag > TT_BETA) return false;
        
        if (entry_depth >= depth) {
            if (entry_flag == TT_EXACT) {
                return true;
            }
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
