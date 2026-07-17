#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <vector>

std::atomic<bool> global_stop{false};
thread_local SearchInfo info;
extern int num_threads; // Defined in uci.cpp

void clear_heuristics() {
    for (int i = 0; i < 64; i++) {
        info.killer_moves[i][0] = Move();
        info.killer_moves[i][1] = Move();
    }
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 64; j++) {
            for (int k = 0; k < 64; k++) { info.history_table[i][j][k] = 0; }
        }
    }
}

// MVV-LVA [Attacker][Victim]
const int mvv_lva[6][6] = {
    {15, 25, 35, 45, 55, 0}, // PAWN
    {14, 24, 34, 44, 54, 0}, // KNIGHT
    {13, 23, 33, 43, 53, 0}, // BISHOP
    {12, 22, 32, 42, 52, 0}, // ROOK
    {11, 21, 31, 41, 51, 0}, // QUEEN
    {10, 20, 30, 40, 50, 0}  // KING
};

long long get_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void check_time() {
    if (info.time_limit > 0 && (info.nodes & 2047) == 0) {
        if (get_time_ms() - info.start_time > info.time_limit) { global_stop = true; }
    }
}

int score_move(Board &board, Move m, Move hash_move, int depth) {
    if (m.move == hash_move.move) { return 1000000; }
    if (m.is_capture()) {
        int attacker = PIECE_NONE;
        for (int p = PAWN; p <= KING; p++) {
            if ((board.piece_bb[p] & board.color_bb[board.side_to_move]) & (1ULL << m.from())) {
                attacker = p;
                break;
            }
        }
        int victim = PIECE_NONE;
        for (int p = PAWN; p <= KING; p++) {
            if ((board.piece_bb[p] & board.color_bb[1 - board.side_to_move]) & (1ULL << m.to())) {
                victim = p;
                break;
            }
        }
        if (attacker != PIECE_NONE && victim != PIECE_NONE) { return 100000 + mvv_lva[attacker][victim]; }
        return 100000;
    }

    // Quiet moves: Killer and History heuristics
    if (depth >= 0 && depth < 64) {
        if (m.move == info.killer_moves[depth][0].move) return 90000;
        if (m.move == info.killer_moves[depth][1].move) return 80000;
    }
    return info.history_table[board.side_to_move][m.from()][m.to()];
}

void sort_moves(Board &board, std::vector<Move> &moves, Move hash_move, int depth) {
    std::vector<int> scores(moves.size());
    for (size_t i = 0; i < moves.size(); i++) { scores[i] = score_move(board, moves[i], hash_move, depth); }
    for (size_t i = 1; i < moves.size(); i++) {
        int j = i;
        while (j > 0 && scores[j - 1] < scores[j]) {
            std::swap(scores[j], scores[j - 1]);
            std::swap(moves[j], moves[j - 1]);
            j--;
        }
    }
}

int quiescence(Board &board, int alpha, int beta) {
    check_time();
    if (global_stop) return 0;
    info.nodes++;

    int tt_score;
    Move hash_move;
    if (TT.probe(board.hash_key, 0, alpha, beta, tt_score, hash_move)) { return tt_score; }

    int stand_pat = evaluate(board);
    if (stand_pat >= beta) {
        TT.store(board.hash_key, 0, beta, TT_BETA, Move());
        return beta;
    }

    // Delta Pruning
    if (stand_pat + 1000 < alpha) { return alpha; }

    if (alpha < stand_pat) alpha = stand_pat;

    std::vector<Move> moves = MoveGen::generate_pseudo_legal_moves(board);
    std::vector<Move> captures;
    for (Move m : moves) {
        if (m.is_capture()) {
            board.make_move(m);
            if (!board.in_check(static_cast<Color>(1 - board.side_to_move))) captures.push_back(m);
            board.unmake_move(m);
        }
    }

    sort_moves(board, captures, Move(), 0);

    for (Move m : captures) {
        board.make_move(m);
        int score = -quiescence(board, -beta, -alpha);
        board.unmake_move(m);

        if (score >= beta) {
            TT.store(board.hash_key, 0, beta, TT_BETA, m);
            return beta;
        }
        if (score > alpha) alpha = score;
    }

    TT.store(board.hash_key, 0, alpha, TT_ALPHA, Move());
    return alpha;
}

int negamax(Board &board, int depth, int alpha, int beta, bool is_null = false) {
    check_time();
    if (global_stop) return 0;
    info.nodes++;

    if (depth == 0) return quiescence(board, alpha, beta);
    if (board.is_draw()) return 0;

    int tt_score;
    Move hash_move;
    if (TT.probe(board.hash_key, depth, alpha, beta, tt_score, hash_move)) { return tt_score; }

    std::vector<Move> moves = MoveGen::generate_legal_moves(board);
    if (moves.empty()) {
        if (board.in_check(board.side_to_move)) return -30000 + (100 - depth);
        return 0; // Stalemate
    }

    bool in_check = board.in_check(board.side_to_move);
    int static_eval = evaluate(board);

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (depth <= 3 && !in_check && !is_null && abs(beta) < 40000) {
        int rfp_margin = 120 * depth;
        if (static_eval - rfp_margin >= beta) {
            return static_eval; // or beta
        }
    }

    // Null Move Pruning
    if (depth >= 3 && !in_check && !is_null) {
        uint64_t non_pawn_pieces = board.color_bb[board.side_to_move] & ~(board.piece_bb[PAWN] | board.piece_bb[KING]);
        if (non_pawn_pieces) {
            board.make_null_move();
            int R = (depth > 6) ? 3 : 2;
            int null_score = -negamax(board, depth - 1 - R, -beta, -beta + 1, true);
            board.unmake_null_move();
            if (global_stop) return 0;
            if (null_score >= beta) return beta;
        }
    }

    sort_moves(board, moves, hash_move, depth);

    int flag = TT_ALPHA;
    int best_score = -50000;
    Move best_move;

    // Futility Pruning
    bool f_prune = false;
    if (depth <= 3 && !in_check && abs(alpha) < 40000) {
        if (static_eval + 150 * depth <= alpha) { f_prune = true; }
    }

    int moves_searched = 0;
    for (Move m : moves) {
        // Futility Pruning: skip quiet moves if we are far behind
        if (f_prune && moves_searched > 0 && !m.is_capture() && m.promoted() == PIECE_NONE &&
            !board.in_check(static_cast<Color>(1 - board.side_to_move))) {
            continue;
        }

        board.make_move(m);
        int score;

        // Late Move Reductions (LMR)
        bool is_quiet = !m.is_capture() && m.promoted() == PIECE_NONE;
        if (depth >= 3 && moves_searched >= 3 && is_quiet && !in_check) {
            int R = (moves_searched > 6) ? 2 : 1;
            score = -negamax(board, depth - 1 - R, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                // Re-search at full depth and full window
                score = -negamax(board, depth - 1, -beta, -alpha);
            }
        } else if (moves_searched == 0) {
            // PV Node
            score = -negamax(board, depth - 1, -beta, -alpha);
        } else {
            // PVS Zero Window Search
            score = -negamax(board, depth - 1, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                score = -negamax(board, depth - 1, -beta, -alpha);
            }
        }

        board.unmake_move(m);

        moves_searched++;

        if (global_stop) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = m;
        }
        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;
        }
        if (alpha >= beta) {
            if (!m.is_capture()) {
                if (depth < 64 && m.move != info.killer_moves[depth][0].move) {
                    info.killer_moves[depth][1] = info.killer_moves[depth][0];
                    info.killer_moves[depth][0] = m;
                }
                info.history_table[board.side_to_move][m.from()][m.to()] += depth * depth;
            }
            TT.store(board.hash_key, depth, beta, TT_BETA, m);
            return beta;
        }
    }

    TT.store(board.hash_key, depth, best_score, flag, best_move);
    return best_score;
}

Move search_worker(Board board, int depth_limit, long long time_limit_ms, bool is_main_thread) {
    info.nodes = 0;
    info.start_time = get_time_ms();
    info.time_limit = time_limit_ms;

    clear_heuristics();

    Move best_move_overall;
    int previous_score = 0;

    for (int depth = 1; depth <= depth_limit; depth++) {
        int tt_score;
        Move hash_move;
        TT.probe(board.hash_key, depth, -50000, 50000, tt_score, hash_move);

        std::vector<Move> moves = MoveGen::generate_legal_moves(board);
        if (moves.empty()) break;

        sort_moves(board, moves, hash_move, depth);

        Move best_move_current = moves[0];
        int best_score = -50000;

        // Aspiration Windows
        int alpha = -50000;
        int beta = 50000;
        if (depth >= 4) {
            alpha = std::max(-50000, previous_score - 50);
            beta = std::min(50000, previous_score + 50);
        }

        while (true) {
            best_score = -50000;
            int current_alpha = alpha; // Keep track of the starting alpha for fail-low checks

            int moves_searched = 0;
            for (Move m : moves) {
                board.make_move(m);
                int score;
                
                if (moves_searched == 0) {
                    score = -negamax(board, depth - 1, -beta, -alpha);
                } else {
                    score = -negamax(board, depth - 1, -alpha - 1, -alpha);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, depth - 1, -beta, -alpha);
                    }
                }
                
                board.unmake_move(m);
                moves_searched++;

                if (global_stop) break;

                if (score > best_score) {
                    best_score = score;
                    best_move_current = m;
                }
                if (score > alpha) { alpha = score; }
            }

            if (global_stop) break;

            // Handle Aspiration Window Failures
            if (best_score <= current_alpha) {
                // Fail Low: the score was worse than expected, widen alpha
                alpha = -50000;
            } else if (best_score >= beta) {
                // Fail High: the score was better than expected, widen beta
                beta = 50000;
            } else {
                // Score falls exactly inside the window, it's exact!
                break;
            }
        }

        if (global_stop) break;

        previous_score = best_score;
        best_move_overall = best_move_current;

        if (is_main_thread) {
            extern bool is_tui;
            if (!is_tui) {
                std::cout << "info depth " << depth << " score cp " << best_score << " nodes " << info.nodes << " pv "
                          << best_move_overall.to_string() << "\n";
            }
        }
    }
    return best_move_overall;
}

Move search(Board board, int depth_limit, long long time_limit_ms) {
    global_stop = false;

    int active_threads = num_threads > 0 ? num_threads : 1;

    std::vector<std::thread> workers;
    // Spawn helper threads (Lazy SMP)
    for (int i = 1; i < active_threads; i++) {
        workers.emplace_back(
            [board, depth_limit, time_limit_ms]() { search_worker(board, depth_limit, time_limit_ms, false); });
    }

    // Main thread does the exact same search, but prints UCI info
    Move best = search_worker(board, depth_limit, time_limit_ms, true);

    // Stop all background threads once the main thread finishes
    global_stop = true;

    for (auto &t : workers) {
        if (t.joinable()) { t.join(); }
    }

    return best;
}
