#include "gendata.h"
#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <sstream>

struct GamePosition {
    std::string fen;
    int score;
};

void generate_data(int num_games, int depth) {
    std::ofstream outfile("data.epd", std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open data.epd" << std::endl;
        return;
    }

    std::mt19937 rng(12345);
    std::cout << "Starting data generation: " << num_games << " games at depth " << depth << std::endl;

    for (int g = 0; g < num_games; g++) {
        Board board;
        board.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        TT.clear();

        std::vector<GamePosition> game_history;
        int result = -1; // -1 ongoing, 0 draw, 1 white win, 2 black win

        // Play 8 random moves to create unique openings
        for (int i = 0; i < 8; i++) {
            std::vector<Move> moves = MoveGen::generate_legal_moves(board);
            if (moves.empty()) break;
            std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
            board.make_move(moves[dist(rng)]);
        }

        while (result == -1 && !global_stop.load()) {
            std::vector<Move> moves = MoveGen::generate_legal_moves(board);
            if (moves.empty()) {
                if (board.in_check(board.side_to_move)) {
                    result = board.side_to_move == WHITE ? 2 : 1;
                } else {
                    result = 0; // Stalemate
                }
                break;
            }

            if (board.half_move_clock >= 100) {
                result = 0; // 50-move rule
                break;
            }

            int reps = 0;
            for (int i = std::max(0, board.history_ply - board.half_move_clock); i < board.history_ply; i++) {
                if (board.history[i].hash_key == board.hash_key) reps++;
            }
            if (reps >= 2) {
                result = 0; // Draw by repetition
                break;
            }

            Move best_move = search(board, depth, -1);
            global_stop = false; // Reset it because search() sets it to true on exit!
            if (best_move.move == 0) { break; }

            int score = evaluate(board);
            // Flip score so it is absolute, not relative to side to move
            if (board.side_to_move == BLACK) score = -score;

            game_history.push_back({board.get_fen(), score});
            board.make_move(best_move);
        }

        if (global_stop.load()) break;

        std::string res_str = "0.5";
        if (result == 1)
            res_str = "1.0";
        else if (result == 2)
            res_str = "0.0";

        for (const auto &pos : game_history) {
            outfile << pos.fen << " c0 \"" << pos.score << "\"; c1 \"" << res_str << "\";\n";
        }

        if ((g + 1) % 10 == 0) { std::cout << "Played " << (g + 1) << " games..." << std::endl; }
    }
    std::cout << "Data generation complete." << std::endl;
    outfile.close();
}
