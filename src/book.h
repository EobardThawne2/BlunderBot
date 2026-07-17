#pragma once
#include <cstdint>
#include <string>
#include "move.h"
#include "board.h"

class OpeningBook {
public:
    static void load(const std::string& filename);
    static Move probe(uint64_t hash_key);
    static bool is_loaded();
};
