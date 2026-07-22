#include "see.h"
#include "magic.h"
#include <algorithm>

const int see_piece_values[6] = {100, 300, 300, 500, 900, 20000};

// Helper function to find the smallest attacker of a given square for a given side.
// It returns the piece type and sets attacker_sq to the square of the attacker.
static int get_smallest_attacker(const Board &board, int sq, Color side, uint64_t occupied, int &attacker_sq) {
    uint64_t pawns = board.piece_bb[PAWN] & board.color_bb[side] & occupied;
    if (pawns) {
        // Pawn attacks are directional. If 'side' is attacking 'sq', we look backward from 'sq'.
        uint64_t attackers = pawn_attacks[1 - side][sq] & pawns;
        if (attackers) {
            attacker_sq = Utils::get_lsb_index(attackers);
            return PAWN;
        }
    }

    uint64_t knights = board.piece_bb[KNIGHT] & board.color_bb[side] & occupied;
    if (knights) {
        uint64_t attackers = knight_attacks[sq] & knights;
        if (attackers) {
            attacker_sq = Utils::get_lsb_index(attackers);
            return KNIGHT;
        }
    }

    uint64_t bishops = (board.piece_bb[BISHOP] | board.piece_bb[QUEEN]) & board.color_bb[side] & occupied;
    if (bishops) {
        uint64_t attackers = get_bishop_attacks(sq, occupied) & bishops;
        if (attackers) {
            attacker_sq = Utils::get_lsb_index(attackers);
            return (Utils::test_bit(board.piece_bb[BISHOP], attacker_sq)) ? BISHOP : QUEEN;
        }
    }

    uint64_t rooks = (board.piece_bb[ROOK] | board.piece_bb[QUEEN]) & board.color_bb[side] & occupied;
    if (rooks) {
        uint64_t attackers = get_rook_attacks(sq, occupied) & rooks;
        if (attackers) {
            attacker_sq = Utils::get_lsb_index(attackers);
            return (Utils::test_bit(board.piece_bb[ROOK], attacker_sq)) ? ROOK : QUEEN;
        }
    }

    uint64_t king = board.piece_bb[KING] & board.color_bb[side] & occupied;
    if (king) {
        uint64_t attackers = king_attacks[sq] & king;
        if (attackers) {
            attacker_sq = Utils::get_lsb_index(attackers);
            return KING;
        }
    }

    return PIECE_NONE;
}

int see(const Board &board, Move move) {
    int gain[32];
    int d = 0;

    int to = move.to();
    int from = move.from();

    int moved_piece = PIECE_NONE;
    for (int p = PAWN; p <= KING; p++) {
        if (Utils::test_bit(board.piece_bb[p], from)) {
            moved_piece = p;
            break;
        }
    }

    int target_piece = PIECE_NONE;
    if (move.is_en_passant()) {
        target_piece = PAWN;
    } else {
        for (int p = PAWN; p <= KING; p++) {
            if (Utils::test_bit(board.piece_bb[p], to)) {
                target_piece = p;
                break;
            }
        }
    }

    if (move.is_castling()) return 0;

    gain[d] = target_piece == PIECE_NONE ? 0 : see_piece_values[target_piece];

    if (move.promoted() != PIECE_NONE) {
        gain[d] += see_piece_values[move.promoted()] - see_piece_values[PAWN];
        moved_piece = move.promoted();
    }

    uint64_t occupied = board.color_bb[WHITE] | board.color_bb[BLACK];
    Color stm = board.side_to_move;

    // Make the initial move on our virtual board
    Utils::clear_bit(occupied, from);
    Utils::set_bit(occupied, to);
    if (move.is_en_passant()) {
        int ep_sq = (stm == WHITE) ? to - 8 : to + 8;
        Utils::clear_bit(occupied, ep_sq);
    }

    stm = (Color)(1 - stm); // Switch side
    int attacker_piece = moved_piece;

    while (true) {
        d++;
        gain[d] =
            see_piece_values[attacker_piece] - gain[d - 1]; // The opponent gains what we moved minus what we gained

        int next_attacker_sq;
        int next_attacker = get_smallest_attacker(board, to, stm, occupied, next_attacker_sq);

        if (next_attacker == PIECE_NONE) { break; }

        // If the King captures, and there are still attackers, we can't capture!
        // We simulate the King capture, check if it's attacked by the OTHER side, and if so,
        // we revert it by not allowing the King capture (breaking out).
        if (next_attacker == KING) {
            // Can the other side attack the king if it captures?
            int dummy_sq;
            if (get_smallest_attacker(board, to, (Color)(1 - stm), occupied, dummy_sq) != PIECE_NONE) { break; }
        }

        attacker_piece = next_attacker;
        Utils::clear_bit(occupied, next_attacker_sq);
        stm = (Color)(1 - stm);
    }

    while (--d) { gain[d - 1] = -std::max(-gain[d - 1], gain[d]); }

    return gain[0];
}
