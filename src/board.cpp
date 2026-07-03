#include "board.h"
#include <iostream>
#include <sstream>
#include "magic.h"
#include "movegen.h"
#include "zobrist.h"

Board::Board() {
    clear();
}

void Board::clear() {
    for (int i = 0; i < 6; i++) piece_bb[i] = 0;
    for (int i = 0; i < 2; i++) color_bb[i] = 0;
    side_to_move = WHITE;
    castling_rights = 0;
    en_passant = SQ_NONE;
    half_move_clock = 0;
    full_move_number = 1;
    history_ply = 0;
    hash_key = 0;
}

void Board::parse_fen(const std::string& fen) {
    clear();
    std::istringstream ss(fen);
    std::string token;
    
    // 1. Piece placement
    ss >> token;
    int rank = 7, file = 0;
    for (char c : token) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += c - '0';
        } else {
            int piece = PIECE_NONE;
            Color col = isupper(c) ? WHITE : BLACK;
            char lc = tolower(c);
            if (lc == 'p') piece = PAWN;
            else if (lc == 'n') piece = KNIGHT;
            else if (lc == 'b') piece = BISHOP;
            else if (lc == 'r') piece = ROOK;
            else if (lc == 'q') piece = QUEEN;
            else if (lc == 'k') piece = KING;
            
            int sq = rank * 8 + file;
            Utils::set_bit(piece_bb[piece], sq);
            Utils::set_bit(color_bb[col], sq);
            file++;
        }
    }
    
    // 2. Side to move
    if (ss >> token) {
        side_to_move = (token == "w") ? WHITE : BLACK;
    }
    
    // 3. Castling rights
    if (ss >> token && token != "-") {
        for (char c : token) {
            if (c == 'K') castling_rights |= WK;
            if (c == 'Q') castling_rights |= WQ;
            if (c == 'k') castling_rights |= BK;
            if (c == 'q') castling_rights |= BQ;
        }
    }
    
    // 4. En passant
    if (ss >> token && token != "-") {
        int file = token[0] - 'a';
        int rank = token[1] - '1';
        en_passant = rank * 8 + file;
    }
    
    // 5. Halfmove clock
    if (ss >> token) {
        half_move_clock = std::stoi(token);
    }
    
    // 6. Fullmove number
    if (ss >> token) {
        full_move_number = std::stoi(token);
    }
    
    hash_key = generate_hash(*this);
}

void Board::print_board() const {
    std::cout << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << "  ";
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            char piece_char = '.';
            
            if (Utils::test_bit(color_bb[WHITE], square)) {
                if (Utils::test_bit(piece_bb[PAWN], square)) piece_char = 'P';
                else if (Utils::test_bit(piece_bb[KNIGHT], square)) piece_char = 'N';
                else if (Utils::test_bit(piece_bb[BISHOP], square)) piece_char = 'B';
                else if (Utils::test_bit(piece_bb[ROOK], square)) piece_char = 'R';
                else if (Utils::test_bit(piece_bb[QUEEN], square)) piece_char = 'Q';
                else if (Utils::test_bit(piece_bb[KING], square)) piece_char = 'K';
            } else if (Utils::test_bit(color_bb[BLACK], square)) {
                if (Utils::test_bit(piece_bb[PAWN], square)) piece_char = 'p';
                else if (Utils::test_bit(piece_bb[KNIGHT], square)) piece_char = 'n';
                else if (Utils::test_bit(piece_bb[BISHOP], square)) piece_char = 'b';
                else if (Utils::test_bit(piece_bb[ROOK], square)) piece_char = 'r';
                else if (Utils::test_bit(piece_bb[QUEEN], square)) piece_char = 'q';
                else if (Utils::test_bit(piece_bb[KING], square)) piece_char = 'k';
            }
            std::cout << piece_char << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";

    std::cout << "Side to move:    " << (side_to_move == WHITE ? "White" : "Black") << "\n";
    std::cout << "Castling rights: " 
              << ((castling_rights & WK) ? 'K' : '-')
              << ((castling_rights & WQ) ? 'Q' : '-')
              << ((castling_rights & BK) ? 'k' : '-')
              << ((castling_rights & BQ) ? 'q' : '-') << "\n";
    std::cout << "En passant:      ";
    if (en_passant == SQ_NONE) {
        std::cout << "-\n";
    } else {
        std::cout << (char)('a' + (en_passant % 8)) << (char)('1' + (en_passant / 8)) << "\n";
    }
    std::cout << "Half-move clock: " << half_move_clock << "\n";
    std::cout << "Full-move number: " << full_move_number << "\n";
    std::cout << "\n";
}

void Board::print_board_tui() const {
    std::cout << "\n";
    
    // Arrays for side panel text
    std::string side_panel[8] = {""};
    side_panel[7] = "\033[1mBLUNDERBOT\033[0m";
    side_panel[6] = "----------";
    side_panel[5] = "Turn:      " + std::string(side_to_move == WHITE ? "White" : "Black");
    
    std::string castling_str = "Castling:  ";
    castling_str += (castling_rights & WK) ? 'K' : '-';
    castling_str += (castling_rights & WQ) ? 'Q' : '-';
    castling_str += (castling_rights & BK) ? 'k' : '-';
    castling_str += (castling_rights & BQ) ? 'q' : '-';
    side_panel[4] = castling_str;
    
    std::string ep_str = "En Passant: ";
    if (en_passant == SQ_NONE) ep_str += "-";
    else ep_str += std::string(1, 'a' + (en_passant % 8)) + std::to_string(1 + (en_passant / 8));
    side_panel[3] = ep_str;
    
    side_panel[2] = "Half-Move:  " + std::to_string(half_move_clock);
    side_panel[1] = "Full-Move:  " + std::to_string(full_move_number);
    
    char hash_buf[32];
    snprintf(hash_buf, sizeof(hash_buf), "Hash:       %I64x", (unsigned long long)hash_key);
    side_panel[0] = hash_buf;

    for (int rank = 7; rank >= 0; --rank) {
        std::cout << "  " << (rank + 1) << " ";
        
        // Draw the top half of the square to make it more square
        for (int file = 0; file < 8; ++file) {
            bool is_light = (rank + file) % 2 != 0;
            std::cout << (is_light ? "\033[48;5;222m" : "\033[48;5;94m") << "     \033[0m";
        }
        std::cout << "\n    ";

        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            bool is_light = (rank + file) % 2 != 0;
            std::string bg = is_light ? "\033[48;5;222m" : "\033[48;5;94m";
            std::string fg = "\033[30m";
            std::string piece_str = "     "; 
            
            if (Utils::test_bit(color_bb[WHITE], square)) {
                fg = "\033[30m"; // Black text (so the outline is black)
                if (Utils::test_bit(piece_bb[PAWN], square)) piece_str = "  \xe2\x99\x99  ";
                else if (Utils::test_bit(piece_bb[KNIGHT], square)) piece_str = "  \xe2\x99\x98  ";
                else if (Utils::test_bit(piece_bb[BISHOP], square)) piece_str = "  \xe2\x99\x97  ";
                else if (Utils::test_bit(piece_bb[ROOK], square)) piece_str = "  \xe2\x99\x96  ";
                else if (Utils::test_bit(piece_bb[QUEEN], square)) piece_str = "  \xe2\x99\x95  ";
                else if (Utils::test_bit(piece_bb[KING], square)) piece_str = "  \xe2\x99\x94  ";
            } else if (Utils::test_bit(color_bb[BLACK], square)) {
                fg = "\033[30m"; // Black text (solid fill)
                if (Utils::test_bit(piece_bb[PAWN], square)) piece_str = "  \xe2\x99\x9f  ";
                else if (Utils::test_bit(piece_bb[KNIGHT], square)) piece_str = "  \xe2\x99\x9e  ";
                else if (Utils::test_bit(piece_bb[BISHOP], square)) piece_str = "  \xe2\x99\x9d  ";
                else if (Utils::test_bit(piece_bb[ROOK], square)) piece_str = "  \xe2\x99\x9c  ";
                else if (Utils::test_bit(piece_bb[QUEEN], square)) piece_str = "  \xe2\x99\x9b  ";
                else if (Utils::test_bit(piece_bb[KING], square)) piece_str = "  \xe2\x99\x9a  ";
            }
            
            std::cout << bg << fg << piece_str << "\033[0m";
        }
        std::cout << "    " << side_panel[rank] << "\n";
        
        // Draw the bottom half of the square to make it more square
        std::cout << "    ";
        for (int file = 0; file < 8; ++file) {
            bool is_light = (rank + file) % 2 != 0;
            std::cout << (is_light ? "\033[48;5;222m" : "\033[48;5;94m") << "     \033[0m";
        }
        std::cout << "\n";
    }
    std::cout << "       a    b    c    d    e    f    g    h\n\n";
}

bool Board::is_square_attacked(int sq, Color attacker_side) const {
    uint64_t occupied = color_bb[WHITE] | color_bb[BLACK];
    if (pawn_attacks[1 - attacker_side][sq] & piece_bb[PAWN] & color_bb[attacker_side]) return true;
    if (knight_attacks[sq] & piece_bb[KNIGHT] & color_bb[attacker_side]) return true;
    if (get_bishop_attacks(sq, occupied) & (piece_bb[BISHOP] | piece_bb[QUEEN]) & color_bb[attacker_side]) return true;
    if (get_rook_attacks(sq, occupied) & (piece_bb[ROOK] | piece_bb[QUEEN]) & color_bb[attacker_side]) return true;
    if (king_attacks[sq] & piece_bb[KING] & color_bb[attacker_side]) return true;
    return false;
}

bool Board::in_check(Color side) const {
    uint64_t king = piece_bb[KING] & color_bb[side];
    if (!king) return false;
    int sq = Utils::get_lsb_index(king);
    return is_square_attacked(sq, side == WHITE ? BLACK : WHITE);
}

void Board::make_move(Move move) {
    history[history_ply].castling_rights = castling_rights;
    history[history_ply].en_passant = en_passant;
    history[history_ply].half_move_clock = half_move_clock;
    history[history_ply].hash_key = hash_key;
    
    int from = move.from();
    int to = move.to();
    int moved_piece = PIECE_NONE;

    for (int i = PAWN; i <= KING; i++) {
        if (Utils::test_bit(piece_bb[i] & color_bb[side_to_move], from)) {
            moved_piece = i;
            break;
        }
    }

    int captured_piece = PIECE_NONE;
    if (move.is_capture()) {
        if (move.is_en_passant()) {
            captured_piece = PAWN;
            int cap_sq = side_to_move == WHITE ? to - 8 : to + 8;
            Utils::clear_bit(piece_bb[PAWN], cap_sq);
            Utils::clear_bit(color_bb[1 - side_to_move], cap_sq);
        } else {
            for (int i = PAWN; i <= KING; i++) {
                if (Utils::test_bit(piece_bb[i] & color_bb[1 - side_to_move], to)) {
                    captured_piece = i;
                    Utils::clear_bit(piece_bb[i], to);
                    Utils::clear_bit(color_bb[1 - side_to_move], to);
                    break;
                }
            }
        }
    }
    history[history_ply].captured_piece = captured_piece;
    history_ply++;

    Utils::clear_bit(piece_bb[moved_piece], from);
    Utils::clear_bit(color_bb[side_to_move], from);
    
    if (move.promoted() != PIECE_NONE) {
        Utils::set_bit(piece_bb[move.promoted()], to);
        Utils::set_bit(color_bb[side_to_move], to);
    } else {
        Utils::set_bit(piece_bb[moved_piece], to);
        Utils::set_bit(color_bb[side_to_move], to);
    }

    if (move.is_castling()) {
        if (to == G1) { Utils::clear_bit(piece_bb[ROOK], H1); Utils::clear_bit(color_bb[WHITE], H1); Utils::set_bit(piece_bb[ROOK], F1); Utils::set_bit(color_bb[WHITE], F1); }
        else if (to == C1) { Utils::clear_bit(piece_bb[ROOK], A1); Utils::clear_bit(color_bb[WHITE], A1); Utils::set_bit(piece_bb[ROOK], D1); Utils::set_bit(color_bb[WHITE], D1); }
        else if (to == G8) { Utils::clear_bit(piece_bb[ROOK], H8); Utils::clear_bit(color_bb[BLACK], H8); Utils::set_bit(piece_bb[ROOK], F8); Utils::set_bit(color_bb[BLACK], F8); }
        else if (to == C8) { Utils::clear_bit(piece_bb[ROOK], A8); Utils::clear_bit(color_bb[BLACK], A8); Utils::set_bit(piece_bb[ROOK], D8); Utils::set_bit(color_bb[BLACK], D8); }
    }

    en_passant = SQ_NONE;
    if (move.is_double_push()) {
        en_passant = side_to_move == WHITE ? to - 8 : to + 8;
    }

    if (moved_piece == PAWN || move.is_capture()) half_move_clock = 0;
    else half_move_clock++;

    if (from == E1 || to == E1) castling_rights &= ~(WK | WQ);
    if (from == E8 || to == E8) castling_rights &= ~(BK | BQ);
    if (from == A1 || to == A1) castling_rights &= ~WQ;
    if (from == H1 || to == H1) castling_rights &= ~WK;
    if (from == A8 || to == A8) castling_rights &= ~BQ;
    if (from == H8 || to == H8) castling_rights &= ~BK;

    if (side_to_move == BLACK) full_move_number++;
    side_to_move = side_to_move == WHITE ? BLACK : WHITE;
    
    hash_key = generate_hash(*this);
}

void Board::unmake_move(Move move) {
    side_to_move = side_to_move == WHITE ? BLACK : WHITE;
    if (side_to_move == BLACK) full_move_number--;

    history_ply--;
    castling_rights = history[history_ply].castling_rights;
    en_passant = history[history_ply].en_passant;
    half_move_clock = history[history_ply].half_move_clock;
    int captured_piece = history[history_ply].captured_piece;
    hash_key = history[history_ply].hash_key;

    int from = move.from();
    int to = move.to();

    if (move.promoted() != PIECE_NONE) {
        Utils::clear_bit(piece_bb[move.promoted()], to);
        Utils::clear_bit(color_bb[side_to_move], to);
        Utils::set_bit(piece_bb[PAWN], from);
        Utils::set_bit(color_bb[side_to_move], from);
    } else {
        for (int i = PAWN; i <= KING; i++) {
            if (Utils::test_bit(piece_bb[i] & color_bb[side_to_move], to)) {
                Utils::clear_bit(piece_bb[i], to);
                Utils::clear_bit(color_bb[side_to_move], to);
                Utils::set_bit(piece_bb[i], from);
                Utils::set_bit(color_bb[side_to_move], from);
                break;
            }
        }
    }

    if (move.is_capture()) {
        if (move.is_en_passant()) {
            int cap_sq = side_to_move == WHITE ? to - 8 : to + 8;
            Utils::set_bit(piece_bb[PAWN], cap_sq);
            Utils::set_bit(color_bb[1 - side_to_move], cap_sq);
        } else {
            Utils::set_bit(piece_bb[captured_piece], to);
            Utils::set_bit(color_bb[1 - side_to_move], to);
        }
    }

    if (move.is_castling()) {
        if (to == G1) { Utils::set_bit(piece_bb[ROOK], H1); Utils::set_bit(color_bb[WHITE], H1); Utils::clear_bit(piece_bb[ROOK], F1); Utils::clear_bit(color_bb[WHITE], F1); }
        else if (to == C1) { Utils::set_bit(piece_bb[ROOK], A1); Utils::set_bit(color_bb[WHITE], A1); Utils::clear_bit(piece_bb[ROOK], D1); Utils::clear_bit(color_bb[WHITE], D1); }
        else if (to == G8) { Utils::set_bit(piece_bb[ROOK], H8); Utils::set_bit(color_bb[BLACK], H8); Utils::clear_bit(piece_bb[ROOK], F8); Utils::clear_bit(color_bb[BLACK], F8); }
        else if (to == C8) { Utils::set_bit(piece_bb[ROOK], A8); Utils::set_bit(color_bb[BLACK], A8); Utils::clear_bit(piece_bb[ROOK], D8); Utils::clear_bit(color_bb[BLACK], D8); }
    }
}

void Board::make_null_move() {
    history[history_ply].castling_rights = castling_rights;
    history[history_ply].en_passant = en_passant;
    history[history_ply].half_move_clock = half_move_clock;
    history[history_ply].hash_key = hash_key;

    en_passant = SQ_NONE;
    if (side_to_move == BLACK) full_move_number++;
    side_to_move = side_to_move == WHITE ? BLACK : WHITE;
    history_ply++;
    hash_key = generate_hash(*this);
}

void Board::unmake_null_move() {
    history_ply--;
    castling_rights = history[history_ply].castling_rights;
    en_passant = history[history_ply].en_passant;
    half_move_clock = history[history_ply].half_move_clock;
    hash_key = history[history_ply].hash_key;

    side_to_move = side_to_move == WHITE ? BLACK : WHITE;
    if (side_to_move == BLACK) full_move_number--;
}
