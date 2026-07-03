#include "zobrist.h"
#include "board.h"
#include "utils.h"
#include <random>

uint64_t zobrist_piece[2][6][64];
uint64_t zobrist_side;
uint64_t zobrist_castling[16];
uint64_t zobrist_en_passant[64];

void init_zobrist() {
    std::mt19937_64 rng(1070372); // Fixed seed
    
    for (int c = WHITE; c <= BLACK; c++) {
        for (int p = PAWN; p <= KING; p++) {
            for (int sq = 0; sq < 64; sq++) {
                zobrist_piece[c][p][sq] = rng();
            }
        }
    }
    
    zobrist_side = rng();
    
    for (int i = 0; i < 16; i++) {
        zobrist_castling[i] = rng();
    }
    
    for (int i = 0; i < 64; i++) {
        zobrist_en_passant[i] = rng();
    }
}

uint64_t generate_hash(const Board& board) {
    uint64_t hash = 0;
    
    for (int c = WHITE; c <= BLACK; c++) {
        for (int p = PAWN; p <= KING; p++) {
            uint64_t bb = board.piece_bb[p] & board.color_bb[c];
            while (bb) {
                int sq = Utils::get_lsb_index(bb);
                Utils::pop_lsb(bb);
                hash ^= zobrist_piece[c][p][sq];
            }
        }
    }
    
    if (board.side_to_move == BLACK) {
        hash ^= zobrist_side;
    }
    
    hash ^= zobrist_castling[board.castling_rights];
    
    if (board.en_passant != SQ_NONE) {
        hash ^= zobrist_en_passant[board.en_passant];
    }
    
    return hash;
}
