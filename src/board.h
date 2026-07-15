#pragma once
#include <cstdint>
#include <string>
#include "bitboard.h"
#include "utils.h"
#include "move.h"

struct BoardState {
    int castling_rights;
    int en_passant;
    int half_move_clock;
    int captured_piece;
    uint64_t hash_key;
};

class Board {
public:
    uint64_t piece_bb[6];
    uint64_t color_bb[2];

    Color side_to_move;
    int castling_rights;
    int en_passant;
    int half_move_clock;
    int full_move_number;
    uint64_t hash_key;

    BoardState history[512];
    int history_ply;

    Board();
    void parse_fen(const std::string& fen);
    void print_board() const;
    void print_board_tui() const;

    bool is_square_attacked(int sq, Color attacker_side) const;
    void make_move(Move move);
    void unmake_move(Move move);
    void make_null_move();
    void unmake_null_move();
    bool in_check(Color side) const;
    bool is_draw() const;

private:
    void clear();
};
