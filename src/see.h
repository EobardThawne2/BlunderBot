#pragma once
#include "board.h"
#include "move.h"

// Returns the Static Exchange Evaluation (SEE) score of a move.
// A positive score means the capture is winning material.
// A negative score means the capture is losing material.
// A zero score means it is an even trade or a quiet move.
int see(const Board &board, Move move);
