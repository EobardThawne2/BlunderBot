#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "tt.h"
#include "see.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <vector>
#include <cmath>

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
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) { info.countermoves[i][j] = Move(); }
    }
}

// MVV-LVA removed in favor of SEE

long long get_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void check_time() {
    if (info.time_limit > 0 && (info.nodes & 2047) == 0) {
        if (get_time_ms() - info.start_time > info.time_limit) { global_stop = true; }
    }
}

int score_move(Board &board, Move m, Move hash_move, int depth, Move prev_move) {
    if (m.move == hash_move.move) { return 1000000; }
    if (m.is_capture()) {
        int see_score = see(board, m);
        if (see_score >= 0)
            return 100000 + see_score; // Good/Equal captures
        else
            return 50000 + see_score; // Bad captures
    }

    // Quiet moves: Killer, Countermove, and History heuristics
    if (depth >= 0 && depth < 64) {
        if (m.move == info.killer_moves[depth][0].move) return 90000;
        if (prev_move.move != 0 && m.move == info.countermoves[prev_move.from()][prev_move.to()].move) return 85000;
        if (m.move == info.killer_moves[depth][1].move) return 80000;
    }
    return info.history_table[board.side_to_move][m.from()][m.to()];
}

void sort_moves(Board &board, std::vector<Move> &moves, Move hash_move, int depth, Move prev_move) {
    std::vector<int> scores(moves.size());
    for (size_t i = 0; i < moves.size(); i++) { scores[i] = score_move(board, moves[i], hash_move, depth, prev_move); }
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
        if (m.is_capture() || m.promoted() == QUEEN) {
            // Prune bad captures in Quiescence Search
            if (m.promoted() != QUEEN && see(board, m) < 0) continue;

            board.make_move(m);
            if (!board.in_check(static_cast<Color>(1 - board.side_to_move))) captures.push_back(m);
            board.unmake_move(m);
        }
    }

    sort_moves(board, captures, Move(), 0, Move());

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

int negamax(Board &board, int depth, int alpha, int beta, bool is_null = false, Move excluded_move = Move(),
            Move prev_move = Move()) {
    check_time();
    if (global_stop) return 0;
    info.nodes++;

    if (depth == 0) return quiescence(board, alpha, beta);
    if (board.is_draw()) return 0;

    int tt_score;
    Move hash_move;
    if (TT.probe(board.hash_key, depth, alpha, beta, tt_score, hash_move)) { return tt_score; }

    bool in_check = board.in_check(board.side_to_move);

    // Internal Iterative Deepening (IID)
    bool is_pv = (beta - alpha > 1);
    if (depth >= 4 && hash_move.move == 0 && !is_null && !in_check && is_pv) {
        negamax(board, depth - 2, alpha, beta, is_null, Move(), prev_move);
        TT.probe(board.hash_key, 0, alpha, beta, tt_score, hash_move);
    }

    std::vector<Move> moves = MoveGen::generate_legal_moves(board);
    if (moves.empty()) {
        if (in_check) return -30000 + (100 - depth);
        return 0; // Stalemate
    }

    int static_eval = evaluate(board);

    // Singular Extension
    bool singular_extension = false;
    if (depth >= 8 && hash_move.move != 0 && !is_null && !in_check && excluded_move.move == 0 &&
        std::abs(beta) < 40000) {
        int rBeta = alpha - 50;
        int rDepth = depth / 2;
        int se_score = negamax(board, rDepth, rBeta - 1, rBeta, false, hash_move, prev_move);
        if (se_score < rBeta) { singular_extension = true; }
    }

    // ProbCut (Multi-Cut)
    if (depth >= 5 && !in_check && !is_null && excluded_move.move == 0 && std::abs(beta) < 40000) {
        int pc_beta = beta + 200;
        int pc_depth = depth - 4;
        int pc_score = negamax(board, pc_depth, pc_beta - 1, pc_beta, false, Move(), prev_move);
        if (pc_score >= pc_beta) { return pc_beta; }
    }

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (depth <= 3 && !in_check && !is_null && std::abs(beta) < 40000) {
        int rfp_margin = 120 * depth;
        if (static_eval - rfp_margin >= beta) {
            return static_eval; // or beta
        }
    }

    // Null Move Pruning
    if (depth >= 3 && !in_check && !is_null && excluded_move.move == 0) {
        uint64_t non_pawn_pieces = board.color_bb[board.side_to_move] & ~(board.piece_bb[PAWN] | board.piece_bb[KING]);
        if (non_pawn_pieces) {
            board.make_null_move();
            int R = (depth > 6) ? 3 : 2;
            int null_score = -negamax(board, depth - 1 - R, -beta, -beta + 1, true, Move(), Move());
            board.unmake_null_move();
            if (global_stop) return 0;
            if (null_score >= beta) return beta;
        }
    }

    sort_moves(board, moves, hash_move, depth, prev_move);

    int flag = TT_ALPHA;
    int best_score = -50000;
    Move best_move;

    // Futility Pruning
    bool f_prune = false;
    if (depth <= 3 && !in_check && std::abs(alpha) < 40000) {
        if (static_eval + 150 * depth <= alpha) { f_prune = true; }
    }

    int moves_searched = 0;
    for (Move m : moves) {
        if (m.move != 0 && m.move == excluded_move.move) continue;

        // Futility Pruning: skip quiet moves if we are far behind
        if (f_prune && moves_searched > 0 && !m.is_capture() && m.promoted() == PIECE_NONE &&
            !board.in_check(static_cast<Color>(1 - board.side_to_move))) {
            continue;
        }

        board.make_move(m);
        int score;

        int ext = (m.move == hash_move.move && singular_extension) ? 1 : 0;
        int new_depth = depth - 1 + ext;

        // Late Move Reductions (LMR)
        bool is_quiet = !m.is_capture() && m.promoted() == PIECE_NONE;

        if (new_depth >= 3 && moves_searched >= 3 && is_quiet && !in_check && ext == 0) {
            bool gives_check = board.in_check(board.side_to_move);
            if (!gives_check) {
                int R = (moves_searched > 6) ? 2 : 1;
                score = -negamax(board, new_depth - R, -alpha - 1, -alpha, false, Move(), m);
                if (score > alpha && score < beta) {
                    // Re-search at full depth and full window
                    score = -negamax(board, new_depth, -beta, -alpha, false, Move(), m);
                }
            } else {
                score = -negamax(board, new_depth, -alpha - 1, -alpha, false, Move(), m);
                if (score > alpha && score < beta) {
                    score = -negamax(board, new_depth, -beta, -alpha, false, Move(), m);
                }
            }
        } else if (moves_searched == 0) {
            // PV Node
            score = -negamax(board, new_depth, -beta, -alpha, false, Move(), m);
        } else {
            // PVS Zero Window Search
            score = -negamax(board, new_depth, -alpha - 1, -alpha, false, Move(), m);
            if (score > alpha && score < beta) { score = -negamax(board, new_depth, -beta, -alpha, false, Move(), m); }
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
                if (prev_move.move != 0) { info.countermoves[prev_move.from()][prev_move.to()] = m; }
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

        sort_moves(board, moves, hash_move, depth, Move());

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
                    score = -negamax(board, depth - 1, -beta, -alpha, false, Move(), m);
                } else {
                    score = -negamax(board, depth - 1, -alpha - 1, -alpha, false, Move(), m);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, depth - 1, -beta, -alpha, false, Move(), m);
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
