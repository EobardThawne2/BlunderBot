#pragma once
#include <cstdint>
#include <string>
#include <cstdlib>
#include <new>
#if defined(_WIN32) || defined(_MSC_VER)
#include <malloc.h>
#endif
#include "bitboard.h"
#include "utils.h"
#include "move.h"
#include "nnue.h"

struct BoardState {
    int castling_rights;
    int en_passant;
    int half_move_clock;
    int captured_piece;
    bool force_nnue_recompute;
    uint64_t hash_key;
};

class Board {
  public:
    void *operator new(size_t size) {
#if defined(_WIN32) || defined(_MSC_VER)
        void *ptr = _aligned_malloc(size, 64);
        if (!ptr) throw std::bad_alloc();
        return ptr;
#else
        void *ptr = nullptr;
        if (posix_memalign(&ptr, 64, size) != 0) throw std::bad_alloc();
        return ptr;
#endif
    }

    void operator delete(void *ptr) {
#if defined(_WIN32) || defined(_MSC_VER)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    uint64_t piece_bb[6];
    uint64_t color_bb[2];

    Color side_to_move;
    int castling_rights;
    int en_passant;
    int half_move_clock;
    int full_move_number;
    uint64_t hash_key;

    BoardState history[2048];
    NNUEdata nnue_state[2048];
    int history_ply;

    Board();
    void parse_fen(const std::string &fen);
    std::string get_fen() const;
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
