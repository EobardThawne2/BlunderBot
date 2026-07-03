#include "tt.h"

TranspositionTable TT;

TranspositionTable::TranspositionTable() {
    resize(32); // Default to 32MB
}

void TranspositionTable::resize(int mb) {
    int bytes = mb * 1024 * 1024;
    int num_entries = bytes / sizeof(TTEntry);
    
    // Find the next power of 2 for size_mask, or just use modulo
    // Bitwise mask is much faster than modulo if size is a power of 2.
    // We'll just force num_entries to be a power of 2.
    int power_of_2_entries = 1;
    while (power_of_2_entries * 2 <= num_entries) {
        power_of_2_entries *= 2;
    }
    
    entries.resize(power_of_2_entries);
    size_mask = power_of_2_entries - 1;
    clear();
}

void TranspositionTable::clear() {
    for (auto& entry : entries) {
        entry.key = 0;
        entry.depth = 0;
        entry.score = 0;
        entry.flag = TT_ALPHA;
        entry.best_move = Move(0);
    }
}

void TranspositionTable::store(uint64_t key, int depth, int score, int flag, Move best_move) {
    int index = key & size_mask;
    
    // Always replace strategy for simplicity
    entries[index].key = key;
    entries[index].depth = depth;
    entries[index].score = score;
    entries[index].flag = flag;
    entries[index].best_move = best_move;
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int& score, Move& best_move) {
    int index = key & size_mask;
    TTEntry& entry = entries[index];
    
    if (entry.key == key) {
        best_move = entry.best_move;
        
        if (entry.depth >= depth) {
            if (entry.flag == TT_EXACT) {
                score = entry.score;
                return true;
            }
            if (entry.flag == TT_ALPHA && entry.score <= alpha) {
                score = alpha;
                return true;
            }
            if (entry.flag == TT_BETA && entry.score >= beta) {
                score = beta;
                return true;
            }
        }
    }
    return false;
}
