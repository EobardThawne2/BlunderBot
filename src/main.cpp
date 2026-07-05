#include "board.h"
#include "magic.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"
#include "uci.h"
#include "nnue.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

bool is_tui = false;

void tui_loop() {
    is_tui = true;
    Board board;
    board.parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::string line;
    
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "Welcome to BlunderBot TUI!\n";
    std::cout << "Commands: [move e.g. e2e4], 'd' (display), 'go' (force engine play), 'quit'.\n";
    board.print_board_tui();

    while (std::cout << "> ", std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "d") {
            board.print_board_tui();
            continue;
        }
        if (line == "go") {
            Move best_move = search(board, 6, 2000);
            board.make_move(best_move);
            std::cout << "\033[2J\033[H";
            std::cout << "Engine played: " << best_move.to_string() << "\n";
            board.print_board_tui();
            continue;
        }

        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        bool valid_move = false;
        for (Move m : moves) {
            if (m.to_string() == line) {
                board.make_move(m);
                valid_move = true;
                break;
            }
        }
        
        if (valid_move) {
            std::cout << "\033[2J\033[H";
            board.print_board_tui();
            
            // Engine automatically responds if it's its turn
            std::cout << "Engine is thinking...\n";
            Move best_move = search(board, 64, 2000);
            if (best_move.move == 0) {
                std::cout << "Game Over!\n";
            } else {
                board.make_move(best_move);
                std::cout << "\033[2J\033[H";
                std::cout << "Engine played: " << best_move.to_string() << "\n";
                board.print_board_tui();
            }
        } else {
            if (!line.empty()) {
                std::cout << "Unknown command or illegal move: " << line << "\n";
            }
        }
    }
}

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#endif

    init_all();
    TT.resize(32);
    nnue_init("nn-62ef826d1a6d.nnue");
    
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "tui" || arg == "play") {
            tui_loop();
            return 0;
        }
    }

    std::cout << "BlunderBot - Type 'uci' for UCI mode or 'tui' for Terminal UI mode.\n";
    std::string line;
    if (std::getline(std::cin, line)) {
        if (line == "tui" || line == "play") {
            tui_loop();
        } else {
            // Pre-feed the line to UCI loop if it was "uci", or just run it anyway
            if (line == "uci") {
                std::cout << "id name Antigravity Engine\n";
                std::cout << "id author DeepMind\n";
                std::cout << "uciok\n";
            }
            uci_loop();
        }
    }
    return 0;
}
