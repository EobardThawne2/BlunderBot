#include "movegen.h"
#include "magic.h"
#include "utils.h"

std::vector<Move> MoveGen::generate_pseudo_legal_moves(const Board &board) {
    std::vector<Move> moves;
    moves.reserve(256);
    Color us = board.side_to_move;
    Color them = (us == WHITE) ? BLACK : WHITE;
    uint64_t empty = ~(board.color_bb[WHITE] | board.color_bb[BLACK]);
    uint64_t occupied = board.color_bb[WHITE] | board.color_bb[BLACK];
    uint64_t enemies = board.color_bb[them];

    // Pawns
    uint64_t pawns = board.piece_bb[PAWN] & board.color_bb[us];
    while (pawns) {
        int sq = Utils::get_lsb_index(pawns);
        Utils::pop_lsb(pawns);
        int r = sq / 8;

        // Single push
        int push_sq = us == WHITE ? sq + 8 : sq - 8;
        if (Utils::test_bit(empty, push_sq)) {
            if ((us == WHITE && r == 6) || (us == BLACK && r == 1)) {
                moves.push_back(Move(sq, push_sq, KNIGHT, false, false, false, false));
                moves.push_back(Move(sq, push_sq, BISHOP, false, false, false, false));
                moves.push_back(Move(sq, push_sq, ROOK, false, false, false, false));
                moves.push_back(Move(sq, push_sq, QUEEN, false, false, false, false));
            } else {
                moves.push_back(Move(sq, push_sq, PIECE_NONE, false, false, false, false));
                // Double push
                if ((us == WHITE && r == 1) || (us == BLACK && r == 6)) {
                    int dp_sq = us == WHITE ? sq + 16 : sq - 16;
                    if (Utils::test_bit(empty, dp_sq)) {
                        moves.push_back(Move(sq, dp_sq, PIECE_NONE, false, false, false, true));
                    }
                }
            }
        }
        // Attacks
        uint64_t attacks = pawn_attacks[us][sq] & enemies;
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            if ((us == WHITE && r == 6) || (us == BLACK && r == 1)) {
                moves.push_back(Move(sq, target, KNIGHT, true, false, false, false));
                moves.push_back(Move(sq, target, BISHOP, true, false, false, false));
                moves.push_back(Move(sq, target, ROOK, true, false, false, false));
                moves.push_back(Move(sq, target, QUEEN, true, false, false, false));
            } else {
                moves.push_back(Move(sq, target, PIECE_NONE, true, false, false, false));
            }
        }
        // En Passant
        if (board.en_passant != SQ_NONE) {
            if (Utils::test_bit(pawn_attacks[us][sq], board.en_passant)) {
                moves.push_back(Move(sq, board.en_passant, PIECE_NONE, true, true, false, false));
            }
        }
    }

    // Knights
    uint64_t knights = board.piece_bb[KNIGHT] & board.color_bb[us];
    while (knights) {
        int sq = Utils::get_lsb_index(knights);
        Utils::pop_lsb(knights);
        uint64_t attacks = knight_attacks[sq] & ~board.color_bb[us];
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            moves.push_back(Move(sq, target, PIECE_NONE, Utils::test_bit(enemies, target), false, false, false));
        }
    }

    // Bishops
    uint64_t bishops = board.piece_bb[BISHOP] & board.color_bb[us];
    while (bishops) {
        int sq = Utils::get_lsb_index(bishops);
        Utils::pop_lsb(bishops);
        uint64_t attacks = get_bishop_attacks(sq, occupied) & ~board.color_bb[us];
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            moves.push_back(Move(sq, target, PIECE_NONE, Utils::test_bit(enemies, target), false, false, false));
        }
    }

    // Rooks
    uint64_t rooks = board.piece_bb[ROOK] & board.color_bb[us];
    while (rooks) {
        int sq = Utils::get_lsb_index(rooks);
        Utils::pop_lsb(rooks);
        uint64_t attacks = get_rook_attacks(sq, occupied) & ~board.color_bb[us];
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            moves.push_back(Move(sq, target, PIECE_NONE, Utils::test_bit(enemies, target), false, false, false));
        }
    }

    // Queens
    uint64_t queens = board.piece_bb[QUEEN] & board.color_bb[us];
    while (queens) {
        int sq = Utils::get_lsb_index(queens);
        Utils::pop_lsb(queens);
        uint64_t attacks = get_queen_attacks(sq, occupied) & ~board.color_bb[us];
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            moves.push_back(Move(sq, target, PIECE_NONE, Utils::test_bit(enemies, target), false, false, false));
        }
    }

    // King
    uint64_t king = board.piece_bb[KING] & board.color_bb[us];
    if (king) {
        int sq = Utils::get_lsb_index(king);
        uint64_t attacks = king_attacks[sq] & ~board.color_bb[us];
        while (attacks) {
            int target = Utils::get_lsb_index(attacks);
            Utils::pop_lsb(attacks);
            moves.push_back(Move(sq, target, PIECE_NONE, Utils::test_bit(enemies, target), false, false, false));
        }

        // Castling
        if (us == WHITE) {
            if (board.castling_rights & WK) {
                if (Utils::test_bit(empty, F1) && Utils::test_bit(empty, G1)) {
                    if (!board.is_square_attacked(E1, BLACK) && !board.is_square_attacked(F1, BLACK)) {
                        moves.push_back(Move(E1, G1, PIECE_NONE, false, false, true, false));
                    }
                }
            }
            if (board.castling_rights & WQ) {
                if (Utils::test_bit(empty, D1) && Utils::test_bit(empty, C1) && Utils::test_bit(empty, B1)) {
                    if (!board.is_square_attacked(E1, BLACK) && !board.is_square_attacked(D1, BLACK)) {
                        moves.push_back(Move(E1, C1, PIECE_NONE, false, false, true, false));
                    }
                }
            }
        } else {
            if (board.castling_rights & BK) {
                if (Utils::test_bit(empty, F8) && Utils::test_bit(empty, G8)) {
                    if (!board.is_square_attacked(E8, WHITE) && !board.is_square_attacked(F8, WHITE)) {
                        moves.push_back(Move(E8, G8, PIECE_NONE, false, false, true, false));
                    }
                }
            }
            if (board.castling_rights & BQ) {
                if (Utils::test_bit(empty, D8) && Utils::test_bit(empty, C8) && Utils::test_bit(empty, B8)) {
                    if (!board.is_square_attacked(E8, WHITE) && !board.is_square_attacked(D8, WHITE)) {
                        moves.push_back(Move(E8, C8, PIECE_NONE, false, false, true, false));
                    }
                }
            }
        }
    }

    return moves;
}

std::vector<Move> MoveGen::generate_legal_moves(Board &board) {
    std::vector<Move> pseudo = generate_pseudo_legal_moves(board);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());

    for (Move m : pseudo) {
        board.make_move(m);
        Color us = board.side_to_move == WHITE ? BLACK : WHITE; // we just moved
        if (!board.in_check(us)) { legal.push_back(m); }
        board.unmake_move(m);
    }
    return legal;
}

uint64_t MoveGen::perft(Board &board, int depth) {
    if (depth == 0) return 1ULL;

    std::vector<Move> moves = generate_legal_moves(board);
    if (depth == 1) return moves.size();

    uint64_t nodes = 0;
    for (Move m : moves) {
        board.make_move(m);
        nodes += perft(board, depth - 1);
        board.unmake_move(m);
    }
    return nodes;
}
