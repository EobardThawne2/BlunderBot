#pragma once
#include <cstdint>

extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];

extern uint64_t bishop_masks[64];
extern uint64_t rook_masks[64];

extern uint64_t bishop_attacks[64][512];
extern uint64_t rook_attacks[64][4096];

extern uint64_t bishop_magic_numbers[64];
extern uint64_t rook_magic_numbers[64];

void init_all();

inline uint64_t get_bishop_attacks(int sq, uint64_t occupancy) {
    occupancy &= bishop_masks[sq];
    occupancy *= bishop_magic_numbers[sq];
    occupancy >>= (64 - 9); // max bishop bits is 9
    return bishop_attacks[sq][occupancy];
}

inline uint64_t get_rook_attacks(int sq, uint64_t occupancy) {
    occupancy &= rook_masks[sq];
    occupancy *= rook_magic_numbers[sq];
    occupancy >>= (64 - 12); // max rook bits is 12
    return rook_attacks[sq][occupancy];
}

inline uint64_t get_queen_attacks(int sq, uint64_t occupancy) {
    return get_bishop_attacks(sq, occupancy) | get_rook_attacks(sq, occupancy);
}
