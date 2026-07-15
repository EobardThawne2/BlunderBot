#include "magic.h"
#include <iostream>
#include <cstring>
#include "zobrist.h"

uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

uint64_t bishop_masks[64];
uint64_t rook_masks[64];

uint64_t bishop_attacks[64][512];
uint64_t rook_attacks[64][4096];

uint64_t bishop_magic_numbers[64];
uint64_t rook_magic_numbers[64];

uint32_t random_state = 1804289383;
uint32_t get_random32() {
    uint32_t number = random_state;
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;
    random_state = number;
    return number;
}
uint64_t get_random64() {
    uint64_t n1 = (uint64_t)(get_random32()) & 0xFFFF;
    uint64_t n2 = (uint64_t)(get_random32()) & 0xFFFF;
    uint64_t n3 = (uint64_t)(get_random32()) & 0xFFFF;
    uint64_t n4 = (uint64_t)(get_random32()) & 0xFFFF;
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}
uint64_t get_magic_number() {
    return get_random64() & get_random64() & get_random64();
}

int count_bits(uint64_t b) {
    int count = 0;
    while (b) {
        count++;
        b &= b - 1;
    }
    return count;
}

int get_lsb_index(uint64_t b) {
    if (b == 0) return -1;
    int count = 0;
    while ((b & 1) == 0) {
        count++;
        b >>= 1;
    }
    return count;
}

uint64_t set_occupancy(int index, int bits_in_mask, uint64_t attack_mask) {
    uint64_t occupancy = 0;
    for (int count = 0; count < bits_in_mask; count++) {
        int square = get_lsb_index(attack_mask);
        attack_mask &= attack_mask - 1;
        if (index & (1 << count)) occupancy |= (1ULL << square);
    }
    return occupancy;
}

uint64_t mask_rook_attacks(int sq) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i <= 6; i++) attacks |= (1ULL << (i * 8 + f));
    for (int i = r - 1; i >= 1; i--) attacks |= (1ULL << (i * 8 + f));
    for (int i = f + 1; i <= 6; i++) attacks |= (1ULL << (r * 8 + i));
    for (int i = f - 1; i >= 1; i--) attacks |= (1ULL << (r * 8 + i));
    return attacks;
}

uint64_t mask_bishop_attacks(int sq) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i <= 6 && j <= 6; i++, j++) attacks |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f + 1; i >= 1 && j <= 6; i--, j++) attacks |= (1ULL << (i * 8 + j));
    for (int i = r + 1, j = f - 1; i <= 6 && j >= 1; i++, j--) attacks |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f - 1; i >= 1 && j >= 1; i--, j--) attacks |= (1ULL << (i * 8 + j));
    return attacks;
}

uint64_t rook_attacks_on_the_fly(int sq, uint64_t block) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i <= 7; i++) {
        attacks |= (1ULL << (i * 8 + f));
        if (block & (1ULL << (i * 8 + f))) break;
    }
    for (int i = r - 1; i >= 0; i--) {
        attacks |= (1ULL << (i * 8 + f));
        if (block & (1ULL << (i * 8 + f))) break;
    }
    for (int i = f + 1; i <= 7; i++) {
        attacks |= (1ULL << (r * 8 + i));
        if (block & (1ULL << (r * 8 + i))) break;
    }
    for (int i = f - 1; i >= 0; i--) {
        attacks |= (1ULL << (r * 8 + i));
        if (block & (1ULL << (r * 8 + i))) break;
    }
    return attacks;
}

uint64_t bishop_attacks_on_the_fly(int sq, uint64_t block) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i <= 7 && j <= 7; i++, j++) {
        attacks |= (1ULL << (i * 8 + j));
        if (block & (1ULL << (i * 8 + j))) break;
    }
    for (int i = r - 1, j = f + 1; i >= 0 && j <= 7; i--, j++) {
        attacks |= (1ULL << (i * 8 + j));
        if (block & (1ULL << (i * 8 + j))) break;
    }
    for (int i = r + 1, j = f - 1; i <= 7 && j >= 0; i++, j--) {
        attacks |= (1ULL << (i * 8 + j));
        if (block & (1ULL << (i * 8 + j))) break;
    }
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; i--, j--) {
        attacks |= (1ULL << (i * 8 + j));
        if (block & (1ULL << (i * 8 + j))) break;
    }
    return attacks;
}

void init_leapers() {
    for (int sq = 0; sq < 64; sq++) {
        int r = sq / 8, f = sq % 8;
        // Pawns
        pawn_attacks[0][sq] = 0; // White
        pawn_attacks[1][sq] = 0; // Black
        if (r < 7) {
            if (f > 0) pawn_attacks[0][sq] |= (1ULL << ((r + 1) * 8 + f - 1));
            if (f < 7) pawn_attacks[0][sq] |= (1ULL << ((r + 1) * 8 + f + 1));
        }
        if (r > 0) {
            if (f > 0) pawn_attacks[1][sq] |= (1ULL << ((r - 1) * 8 + f - 1));
            if (f < 7) pawn_attacks[1][sq] |= (1ULL << ((r - 1) * 8 + f + 1));
        }

        // Knights
        knight_attacks[sq] = 0;
        int k_moves[8][2] = {{2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};
        for (int i = 0; i < 8; i++) {
            int nr = r + k_moves[i][0], nf = f + k_moves[i][1];
            if (nr >= 0 && nr <= 7 && nf >= 0 && nf <= 7) knight_attacks[sq] |= (1ULL << (nr * 8 + nf));
        }

        // Kings
        king_attacks[sq] = 0;
        int king_moves[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
        for (int i = 0; i < 8; i++) {
            int nr = r + king_moves[i][0], nf = f + king_moves[i][1];
            if (nr >= 0 && nr <= 7 && nf >= 0 && nf <= 7) king_attacks[sq] |= (1ULL << (nr * 8 + nf));
        }
    }
}

uint64_t find_magic_number(int sq, int relevant_bits, bool bishop) {
    uint64_t occupancies[4096];
    uint64_t attacks[4096];
    uint64_t used_attacks[4096];

    uint64_t mask = bishop ? mask_bishop_attacks(sq) : mask_rook_attacks(sq);
    int occupancy_indicies = 1 << relevant_bits;

    for (int i = 0; i < occupancy_indicies; i++) {
        occupancies[i] = set_occupancy(i, relevant_bits, mask);
        attacks[i] =
            bishop ? bishop_attacks_on_the_fly(sq, occupancies[i]) : rook_attacks_on_the_fly(sq, occupancies[i]);
    }

    for (int random_count = 0; random_count < 100000000; random_count++) {
        uint64_t magic = get_magic_number();
        if (count_bits((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        memset(used_attacks, 0, sizeof(used_attacks));
        bool fail = false;
        int shift = 64 - relevant_bits; // Flat shift, not optimized per square for brevity

        for (int i = 0; i < occupancy_indicies; i++) {
            int magic_index = (occupancies[i] * magic) >> shift;
            if (used_attacks[magic_index] == 0) {
                used_attacks[magic_index] = attacks[i];
            } else if (used_attacks[magic_index] != attacks[i]) {
                fail = true;
                break;
            }
        }
        if (!fail) return magic;
    }
    std::cout << "Magic failed for sq " << sq << "\\n";
    return 0;
}

void init_sliders(bool bishop) {
    for (int sq = 0; sq < 64; sq++) {
        bishop ? bishop_masks[sq] = mask_bishop_attacks(sq) : rook_masks[sq] = mask_rook_attacks(sq);
        uint64_t mask = bishop ? bishop_masks[sq] : rook_masks[sq];
        int relevant_bits = bishop ? 9 : 12; // Flat 9/12 to make array lookup uniform
        int occupancy_indicies = 1 << relevant_bits;

        uint64_t occupancies[4096];
        uint64_t attacks_on_the_fly[4096];

        for (int i = 0; i < occupancy_indicies; i++) {
            occupancies[i] = set_occupancy(i, relevant_bits, mask);
            attacks_on_the_fly[i] =
                bishop ? bishop_attacks_on_the_fly(sq, occupancies[i]) : rook_attacks_on_the_fly(sq, occupancies[i]);
        }

        uint64_t magic = find_magic_number(sq, relevant_bits, bishop);
        if (bishop)
            bishop_magic_numbers[sq] = magic;
        else
            rook_magic_numbers[sq] = magic;

        int shift = 64 - relevant_bits;
        for (int i = 0; i < occupancy_indicies; i++) {
            int magic_index = (occupancies[i] * magic) >> shift;
            if (bishop)
                bishop_attacks[sq][magic_index] = attacks_on_the_fly[i];
            else
                rook_attacks[sq][magic_index] = attacks_on_the_fly[i];
        }
    }
}

void init_all() {
    init_leapers();
    init_sliders(true);  // bishops
    init_sliders(false); // rooks
    init_zobrist();      // Initialize hash keys!
}
