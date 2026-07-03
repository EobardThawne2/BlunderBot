#include "evaluate.h"
#include "utils.h"
#include "movegen.h"
#include "magic.h"
#include <algorithm>

// Tapered evaluation weights (Middlegame, Endgame)
const int PieceVal[2][6] = {
    { 82, 337, 365, 477, 1025, 0 },
    { 94, 281, 297, 512,  936, 0 }
};

// PSTs (only for white, flip for black)
// Pawn
const int PST_Pawn_MG[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     98, 134,  61,  95,  68, 126,  34, -11,
     -6,   7,  26,  31,  65,  56,  25, -20,
    -14,  13,   6,  21,  23,  12,  17, -23,
    -27,  -2,  -5,  12,  17,   6,  10, -25,
    -26,  -4,  -4, -10,   3,   3,  33, -12,
    -35,  -1, -20, -23, -15,  24,  38, -22,
      0,   0,   0,   0,   0,   0,   0,   0
};
const int PST_Pawn_EG[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0
};

// Knight
const int PST_Knight_MG[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  28,   -16,
     -29, -53, -12,  -3,  -1,  18, -14,   -19,
    -105, -21, -58, -33, -17, -28, -19,   -23
};
const int PST_Knight_EG[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,   2, -20, -23, -44,
    -29, -51, -23, -38, -22, -18, -50, -64
};

// Bishop
const int PST_Bishop_MG[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21
};
const int PST_Bishop_EG[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3,  13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17
};

// Rook
const int PST_Rook_MG[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26
};
const int PST_Rook_EG[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20
};

// Queen
const int PST_Queen_MG[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50
};
const int PST_Queen_EG[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  12,  11,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41
};

// King
const int PST_King_MG[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14
};
const int PST_King_EG[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

const int* PST_MG[6] = { PST_Pawn_MG, PST_Knight_MG, PST_Bishop_MG, PST_Rook_MG, PST_Queen_MG, PST_King_MG };
const int* PST_EG[6] = { PST_Pawn_EG, PST_Knight_EG, PST_Bishop_EG, PST_Rook_EG, PST_Queen_EG, PST_King_EG };

const int GamePhase[6] = { 0, 1, 1, 2, 4, 0 }; // P, N, B, R, Q, K
const int TotalPhase = 24;

// Bonus/Penalty
const int PASSED_PAWN_BONUS[2][8] = {
    { 0, 10, 30, 50, 75, 120, 180, 0 }, // MG
    { 0, 20, 50, 80, 120, 180, 240, 0 } // EG
};
const int ISOLATED_PAWN_PENALTY[2] = { 15, 20 };
const int DOUBLED_PAWN_PENALTY[2] = { 11, 15 };
const int KING_SHIELD_BONUS = 15;

int evaluate(const Board& board) {
    int mg_score[2] = { 0, 0 };
    int eg_score[2] = { 0, 0 };
    int phase = TotalPhase;

    // Pawns for both sides to calculate structure
    uint64_t white_pawns = board.piece_bb[PAWN] & board.color_bb[WHITE];
    uint64_t black_pawns = board.piece_bb[PAWN] & board.color_bb[BLACK];

    for (int col = WHITE; col <= BLACK; col++) {
        for (int p = PAWN; p <= KING; p++) {
            uint64_t bb = board.piece_bb[p] & board.color_bb[col];
            while (bb) {
                int sq = Utils::get_lsb_index(bb);
                Utils::clear_bit(bb, sq);

                // Phase calculation
                phase -= GamePhase[p];

                // Material
                mg_score[col] += PieceVal[0][p];
                eg_score[col] += PieceVal[1][p];

                // PST
                int pst_sq = (col == WHITE) ? (sq ^ 56) : sq; 
                mg_score[col] += PST_MG[p][pst_sq];
                eg_score[col] += PST_EG[p][pst_sq];

                // Advanced Pawn Structure
                if (p == PAWN) {
                    int rank = sq / 8;
                    int file = sq % 8;
                    int rel_rank = (col == WHITE) ? rank : 7 - rank;
                    
                    uint64_t file_mask = 0x0101010101010101ULL << file;
                    uint64_t adj_files = 0;
                    if (file > 0) adj_files |= (file_mask >> 1);
                    if (file < 7) adj_files |= (file_mask << 1);

                    // Doubled Pawn
                    if (board.piece_bb[PAWN] & board.color_bb[col] & file_mask & ~(1ULL << sq)) {
                        mg_score[col] -= DOUBLED_PAWN_PENALTY[0];
                        eg_score[col] -= DOUBLED_PAWN_PENALTY[1];
                    }

                    // Isolated Pawn
                    if (!(board.piece_bb[PAWN] & board.color_bb[col] & adj_files)) {
                        mg_score[col] -= ISOLATED_PAWN_PENALTY[0];
                        eg_score[col] -= ISOLATED_PAWN_PENALTY[1];
                    }

                    // Passed Pawn
                    uint64_t enemy_pawns = (col == WHITE) ? black_pawns : white_pawns;
                    uint64_t passed_mask = 0;
                    if (col == WHITE) {
                        for (int r = rank + 1; r < 8; r++) passed_mask |= (0x0101010101010101ULL << r);
                    } else {
                        for (int r = rank - 1; r >= 0; r--) passed_mask |= (0x0101010101010101ULL << r);
                    }
                    passed_mask &= (file_mask | adj_files);
                    if (!(enemy_pawns & passed_mask)) {
                        mg_score[col] += PASSED_PAWN_BONUS[0][rel_rank];
                        eg_score[col] += PASSED_PAWN_BONUS[1][rel_rank];
                    }
                }
                
                // King Safety (Pawn Shield)
                if (p == KING) {
                    int rank = sq / 8;
                    int file = sq % 8;
                    // Only care if king is castled or on edge
                    if (file < 3 || file > 4) {
                        uint64_t shield_mask = 0;
                        if (col == WHITE && rank < 2) {
                            shield_mask = (0x0101010101010101ULL << (rank+1)) & ((0x0101010101010101ULL << file) | (file>0 ? 0x0101010101010101ULL << (file-1) : 0) | (file<7 ? 0x0101010101010101ULL << (file+1) : 0));
                        } else if (col == BLACK && rank > 5) {
                            shield_mask = (0x0101010101010101ULL << (rank-1)) & ((0x0101010101010101ULL << file) | (file>0 ? 0x0101010101010101ULL << (file-1) : 0) | (file<7 ? 0x0101010101010101ULL << (file+1) : 0));
                        }
                        
                        int shield_pawns = Utils::count_bits((col == WHITE ? white_pawns : black_pawns) & shield_mask);
                        mg_score[col] += shield_pawns * KING_SHIELD_BONUS;
                    }
                }
            }
        }
    }

    // Piece Mobility
    uint64_t occupied = board.color_bb[WHITE] | board.color_bb[BLACK];
    for (int col = WHITE; col <= BLACK; col++) {
        // Knights
        uint64_t knights = board.piece_bb[KNIGHT] & board.color_bb[col];
        while (knights) {
            int sq = Utils::get_lsb_index(knights);
            Utils::clear_bit(knights, sq);
            uint64_t attacks = knight_attacks[sq] & ~board.color_bb[col];
            int mobility = Utils::count_bits(attacks);
            mg_score[col] += (mobility - 4) * 4;
            eg_score[col] += (mobility - 4) * 4;
        }
        // Bishops
        uint64_t bishops = board.piece_bb[BISHOP] & board.color_bb[col];
        while (bishops) {
            int sq = Utils::get_lsb_index(bishops);
            Utils::clear_bit(bishops, sq);
            uint64_t attacks = get_bishop_attacks(sq, occupied) & ~board.color_bb[col];
            int mobility = Utils::count_bits(attacks);
            mg_score[col] += (mobility - 7) * 3;
            eg_score[col] += (mobility - 7) * 3;
        }
        // Rooks
        uint64_t rooks = board.piece_bb[ROOK] & board.color_bb[col];
        while (rooks) {
            int sq = Utils::get_lsb_index(rooks);
            Utils::clear_bit(rooks, sq);
            uint64_t attacks = get_rook_attacks(sq, occupied) & ~board.color_bb[col];
            int mobility = Utils::count_bits(attacks);
            mg_score[col] += (mobility - 7) * 2;
            eg_score[col] += (mobility - 7) * 2;
        }
    }

    // Tapered Evaluation Phase
    phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;
    if (phase < 0) phase = 0;
    if (phase > 256) phase = 256;

    int mg_eval = mg_score[board.side_to_move] - mg_score[1 - board.side_to_move];
    int eg_eval = eg_score[board.side_to_move] - eg_score[1 - board.side_to_move];

    int eval = (mg_eval * phase + eg_eval * (256 - phase)) / 256;

    // Tempo bonus
    eval += 20;

    return eval;
}
