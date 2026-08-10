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

    Board *non_const_board = const_cast<Board *>(&board);
    NNUEdata *current_nnue = &non_const_board->nnue_state[board.history_ply];
    NNUEdata *nnue_data[3] = {current_nnue, nullptr, nullptr};

    if (board.history_ply > 0 && !board.history[board.history_ply - 1].force_nnue_recompute) {
        nnue_data[1] = &non_const_board->nnue_state[board.history_ply - 1];
    }

    int score = nnue_evaluate_incremental(board.side_to_move, pieces, squares, nnue_data);

    // Prevent the score from exceeding mate bounds
    if (score > 30000) score = 30000;
    if (score < -30000) score = -30000;

    return score;
}
