#include "book.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

struct BookEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

static std::vector<BookEntry> book_entries;
static bool loaded = false;

void OpeningBook::load(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "info string Warning: Could not load opening book " << filename << "\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size % sizeof(BookEntry) != 0) {
        std::cerr << "info string Warning: Opening book size is corrupted.\n";
        return;
    }

    size_t num_entries = file_size / sizeof(BookEntry);
    book_entries.resize(num_entries);

    if (file.read(reinterpret_cast<char *>(book_entries.data()), file_size)) {
        loaded = true;
        std::cerr << "info string Loaded custom opening book with " << num_entries << " positions.\n";
    } else {
        std::cerr << "info string Warning: Failed to read opening book.\n";
    }
}

bool OpeningBook::is_loaded() {
    return loaded;
}

Move OpeningBook::probe(uint64_t hash_key) {
    if (!loaded || book_entries.empty()) return Move();

    // Binary search for the first matching key
    int l = 0, r = book_entries.size() - 1;
    int first_match = -1;

    while (l <= r) {
        int m = l + (r - l) / 2;
        if (book_entries[m].key == hash_key) {
            first_match = m;
            r = m - 1; // Keep searching left for the first occurrence
        } else if (book_entries[m].key < hash_key) {
            l = m + 1;
        } else {
            r = m - 1;
        }
    }

    if (first_match == -1) return Move();

    // Collect all moves for this position
    std::vector<BookEntry> candidates;
    for (int i = first_match; i < book_entries.size(); i++) {
        if (book_entries[i].key != hash_key) break;
        candidates.push_back(book_entries[i]);
    }

    if (candidates.empty()) return Move();

    // Simple roulette wheel selection based on weight
    int total_weight = 0;
    for (const auto &entry : candidates) { total_weight += entry.weight; }

    if (total_weight == 0) return Move(candidates[0].move);

    // Naive pseudo-random based on current time (simple but effective for books)
    int r_val = rand() % total_weight;
    int current_weight = 0;

    for (const auto &entry : candidates) {
        current_weight += entry.weight;
        if (r_val < current_weight) { return Move(entry.move); }
    }

    return Move(candidates[0].move);
}
