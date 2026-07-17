#include "board.h"
#include "movegen.h"
#include "zobrist.h"
#include "magic.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>

struct BookEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <openings.txt> <output.bin>\n";
        return 1;
    }

    std::string txt_file = argv[1];
    std::string bin_file = argv[2];

    init_all();

    std::map<uint64_t, std::map<uint16_t, int>> entries;

    std::ifstream infile(txt_file);
    if (!infile.is_open()) {
        std::cerr << "Could not open " << txt_file << "\n";
        return 1;
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;

        Board board;
        board.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        std::istringstream ss(line);
        std::string move_str;
        while (ss >> move_str) {
            std::vector<Move> legal_moves = MoveGen::generate_legal_moves(board);
            bool found = false;
            Move matched_move;

            for (Move m : legal_moves) {
                if (m.to_string() == move_str) {
                    found = true;
                    matched_move = m;
                    break;
                }
            }

            if (!found) {
                std::cerr << "Warning: Move '" << move_str << "' not found in position!\n";
                std::cerr << "Legal moves: ";
                for (Move m : legal_moves) std::cerr << m.to_string() << " ";
                std::cerr << "\n";
                break;
            }

            // We use BlunderBot's native hash and move encoding
            uint64_t key = board.hash_key;
            uint16_t encoded_move = (uint16_t)(matched_move.move & 0xFFFF); // 16 bits is enough since our move struct uses bits 0-19, wait...
            // Our Move struct uses up to bit 19!
            // Wait, bits 0-15 cover from, to, and promoted.
            // is_capture (16), is_en_passant (17), is_castling (18), is_double_push (19).
            // Do we need the flags? The engine can just match (from, to, promoted) in legal moves!
            // So we only need bits 0-15 to identify the move uniquely.
            
            entries[key][encoded_move]++;

            board.make_move(matched_move);
        }
    }

    std::vector<BookEntry> flat_entries;
    for (auto const& [key, moves] : entries) {
        for (auto const& [move, weight] : moves) {
            BookEntry entry;
            entry.key = key;
            entry.move = move;
            entry.weight = (uint16_t)weight;
            entry.learn = 0;
            flat_entries.push_back(entry);
        }
    }

    // Sort by key for binary search
    std::sort(flat_entries.begin(), flat_entries.end(), [](const BookEntry& a, const BookEntry& b) {
        return a.key < b.key;
    });

    std::ofstream outfile(bin_file, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Could not write to " << bin_file << "\n";
        return 1;
    }

    for (const BookEntry& entry : flat_entries) {
        outfile.write(reinterpret_cast<const char*>(&entry), sizeof(BookEntry));
    }

    std::cout << "Successfully compiled " << flat_entries.size() << " book entries into " << bin_file << "!\n";
    return 0;
}
