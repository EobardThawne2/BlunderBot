#include "uci.h"
#include "search.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void parse_position(Board& board, std::istringstream& ss) {
    std::string token;
    ss >> token;

    if (token == "startpos") {
        board.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token; // consume "moves" if it exists
    } else if (token == "fen") {
        std::string fen = "";
        for (int i = 0; i < 6; i++) {
            ss >> token;
            fen += token + " ";
        }
        board.parse_fen(fen);
        ss >> token; // consume "moves" if it exists
    }

    if (token == "moves") {
        while (ss >> token) {
            std::vector<Move> moves = MoveGen::generate_legal_moves(board);
            for (Move m : moves) {
                if (m.to_string() == token) {
                    board.make_move(m);
                    break;
                }
            }
        }
    }
}

void parse_go(Board& board, std::istringstream& ss) {
    std::string token;
    int depth = 64;
    long long wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0;

    while (ss >> token) {
        if (token == "depth") ss >> depth;
        else if (token == "wtime") ss >> wtime;
        else if (token == "btime") ss >> btime;
        else if (token == "winc") ss >> winc;
        else if (token == "binc") ss >> binc;
        else if (token == "movetime") ss >> movetime;
    }

    long long time_limit = -1;
    if (movetime > 0) {
        time_limit = movetime - 50;
    } else {
        long long time_left = (board.side_to_move == WHITE) ? wtime : btime;
        long long inc = (board.side_to_move == WHITE) ? winc : binc;
        if (time_left > 0) {
            time_limit = (time_left / 20) + (inc / 2);
        }
    }

    Move best = search(board, depth, time_limit);
    std::cout << "bestmove " << best.to_string() << std::endl;
}

#include <thread>
int num_threads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1;


void uci_loop() {
    Board board;
    board.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::string line;
    
    // Some GUIs expect the first output immediately
    std::cout << "id name BlunderBot" << std::endl;
    std::cout << "option name Threads type spin default " << num_threads << " min 1 max 128" << std::endl;
    std::cout << "uciok" << std::endl;

    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "uci") {
            std::cout << "id name BlunderBot" << std::endl;
            std::cout << "option name Threads type spin default " << num_threads << " min 1 max 128" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (token == "setoption") {
            std::string name_token, option_name, value_token;
            int option_value;
            ss >> name_token >> option_name >> value_token >> option_value;
            if (name_token == "name" && option_name == "Threads" && value_token == "value") {
                if (option_value >= 1) num_threads = option_value;
            }
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            TT.clear();
        } else if (token == "position") {
            parse_position(board, ss);
        } else if (token == "go") {
            parse_go(board, ss);
        } else if (token == "stop") {
            global_stop = true;
        } else if (token == "quit") {
            break;
        } else if (token == "d") {
            board.print_board();
        } else if (token == "perft") {
            int depth = 1;
            if (ss >> depth) {
                std::cout << "DEBUG: starting perft " << depth << "\n";
                uint64_t nodes = MoveGen::perft(board, depth);
                std::cout << "Nodes searched: " << nodes << "\n";
            } else {
                std::cout << "DEBUG: perft depth parsing failed\n";
            }
        }
    }
}
