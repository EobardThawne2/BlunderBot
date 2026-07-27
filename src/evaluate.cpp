#include "evaluate.h"
#include "utils.h"
#include "nnue.h"
#include <iostream>

// nnue-probe pieces mapping
//     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
//     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12
int get_nnue_piece(int piece, int color) {
    if (color == WHITE) {
        if (piece == KING) return 1;
        if (piece == QUEEN) return 2;
        if (piece == ROOK) return 3;
        if (piece == BISHOP) return 4;
        if (piece == KNIGHT) return 5;
        if (piece == PAWN) return 6;
    } else {
        if (piece == KING) return 7;
        if (piece == QUEEN) return 8;
        if (piece == ROOK) return 9;
        if (piece == BISHOP) return 10;
        if (piece == KNIGHT) return 11;
        if (piece == PAWN) return 12;
    }
    return 0;
}

int evaluate(const Board &board) {
    int pieces[33];
    int squares[33];
    int idx = 2; // 0 and 1 are reserved for the kings

    for (int col = WHITE; col <= BLACK; col++) {
        for (int p = PAWN; p <= KING; p++) {
            uint64_t bb = board.piece_bb[p] & board.color_bb[col];
            while (bb) {
                int sq = Utils::get_lsb_index(bb);
                Utils::clear_bit(bb, sq);

                int nnue_p = get_nnue_piece(p, col);

                if (p == KING) {
                    if (col == WHITE) {
                        pieces[0] = nnue_p;
                        squares[0] = sq;
                    } else {
                        pieces[1] = nnue_p;
                        squares[1] = sq;
                    }
                } else {
                    pieces[idx] = nnue_p;
                    squares[idx] = sq;
                    idx++;
                }
            }
        }
    }

    // Null-terminate the pieces array
    pieces[idx] = 0;
    squares[idx] = 0;

    // Calculate Game Phase
    // Knights = 1, Bishops = 1, Rooks = 2, Queens = 4 (Total = 24 max)
    int phase = 0;
    phase += 1 * Utils::count_bits(board.piece_bb[KNIGHT]);
    phase += 1 * Utils::count_bits(board.piece_bb[BISHOP]);
    phase += 2 * Utils::count_bits(board.piece_bb[ROOK]);
    phase += 4 * Utils::count_bits(board.piece_bb[QUEEN]);

    // Clamp phase between 0 (Endgame) and 24 (Opening)
    if (phase > 24) phase = 24;

    // nnue_evaluate returns the score relative to the side to move.
    int raw_score = nnue_evaluate(board.side_to_move, pieces, squares);

    // Dynamic Endgame Aggression (Phase-based weight shifting)
    // multiplier = 1.0 at phase 24 (opening)
    // multiplier = 1.3 at phase 0 (endgame)
    double scale = 1.3 - (0.3 * phase / 24.0);
    int score = (int)(raw_score * scale);

    // Prevent the score from exceeding mate bounds
    if (score > 30000) score = 30000;
    if (score < -30000) score = -30000;

    return score;
}
