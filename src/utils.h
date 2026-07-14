#pragma once
#include <cstdint>
#include <iostream>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace Utils {
    inline void set_bit(uint64_t& bb, int square) {
        bb |= (1ULL << square);
    }

    inline void clear_bit(uint64_t& bb, int square) {
        bb &= ~(1ULL << square);
    }

    inline bool test_bit(uint64_t bb, int square) {
        return (bb & (1ULL << square)) != 0;
    }

    inline int get_lsb_index(uint64_t bb) {
        if (bb == 0) return -1;
#if defined(_MSC_VER)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return static_cast<int>(index);
#else
        return __builtin_ctzll(bb); // More efficient way using compiler intrinsics
#endif
    }

    inline int count_bits(uint64_t bb) {
#if defined(_MSC_VER)
        return static_cast<int>(__popcnt64(bb));
#else
        return __builtin_popcountll(bb);
#endif
    }


    inline void pop_lsb(uint64_t& bb) {
        bb &= bb - 1;
    }

    inline void print_bitboard(uint64_t bb) {
        std::cout << "\n";
        for (int rank = 7; rank >= 0; --rank) {
            std::cout << (rank + 1) << "  ";
            for (int file = 0; file < 8; ++file) {
                int square = rank * 8 + file;
                std::cout << (test_bit(bb, square) ? "1 " : ". ");
            }
            std::cout << "\n";
        }
        std::cout << "\n   a b c d e f g h\n\n";
    }
}
