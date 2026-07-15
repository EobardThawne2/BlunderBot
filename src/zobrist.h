#pragma once
#include <cstdint>
#include "bitboard.h"

// Forward declaration to avoid circular include if not needed
class Board;

extern uint64_t zobrist_piece[2][6][64]; // [color][piece][square]
extern uint64_t zobrist_side;
extern uint64_t zobrist_castling[16];
extern uint64_t zobrist_en_passant[64];

void init_zobrist();
uint64_t generate_hash(const Board &board);
