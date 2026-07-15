#pragma once
#include <cstdint>
#include <string>
#include "bitboard.h"

// Move Bitpacking (32-bit integer)
// 0-5:   from square (6 bits)
// 6-11:  to square (6 bits)
// 12-15: promoted piece (4 bits) - PIECE_NONE if not promotion
// 16:    is_capture (1 bit)
// 17:    is_en_passant (1 bit)
// 18:    is_castling (1 bit)
// 19:    is_double_push (1 bit)
struct Move {
    uint32_t move;

    Move() : move(0) {}
    Move(uint32_t m) : move(m) {}
    Move(int from, int to, int promoted, bool capture, bool ep, bool castling, bool dp) {
        move = (from & 0x3F) | ((to & 0x3F) << 6) | ((promoted & 0xF) << 12) | (capture ? 0x10000 : 0) |
               (ep ? 0x20000 : 0) | (castling ? 0x40000 : 0) | (dp ? 0x80000 : 0);
    }

    int from() const { return move & 0x3F; }
    int to() const { return (move >> 6) & 0x3F; }
    int promoted() const { return (move >> 12) & 0xF; }
    bool is_capture() const { return (move & 0x10000) != 0; }
    bool is_en_passant() const { return (move & 0x20000) != 0; }
    bool is_castling() const { return (move & 0x40000) != 0; }
    bool is_double_push() const { return (move & 0x80000) != 0; }

    std::string to_string() const {
        std::string s = "";
        s += (char)('a' + (from() % 8));
        s += (char)('1' + (from() / 8));
        s += (char)('a' + (to() % 8));
        s += (char)('1' + (to() / 8));
        if (promoted() != PIECE_NONE) {
            if (promoted() == KNIGHT)
                s += 'n';
            else if (promoted() == BISHOP)
                s += 'b';
            else if (promoted() == ROOK)
                s += 'r';
            else if (promoted() == QUEEN)
                s += 'q';
        }
        return s;
    }

    bool operator==(const Move &other) const { return move == other.move; }
    bool operator!=(const Move &other) const { return move != other.move; }
};
