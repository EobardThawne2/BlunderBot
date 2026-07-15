#pragma once
#include <vector>
#include "move.h"
#include "board.h"

class MoveGen {
public:
    static std::vector<Move> generate_pseudo_legal_moves(const Board& board);
    static std::vector<Move> generate_legal_moves(Board& board);
    static uint64_t perft(Board& board, int depth);
};
