#include "GoGame.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
const int DR[4] = {-1, 1, 0, 0};
const int DC[4] = {0, 0, -1, 1};
const char* COLUMNS = "ABCDEFGHJKLMNOPQRST";  // no 'I', as in standard Go notation
}

GoGame::GoGame(int size, double komi)
    : size(size), komi(komi), board(size, std::vector<char>(size, EMPTY)),
      dead(size, std::vector<bool>(size, false)) {
    history.insert(serialize(board));
}

void GoGame::printBoard(bool highlightLast) const {
    int lr = -1, lc = -1;
    if (highlightLast && hasLastStone()) { lr = moves.back().row; lc = moves.back().col; }
    std::cout << "\n   ";
    for (int c = 0; c < size; ++c) std::cout << COLUMNS[c] << ' ';
    std::cout << '\n';
    for (int r = 0; r < size; ++r) {
        int label = size - r;
        if (label < 10) std::cout << ' ';
        std::cout << label;
        for (int c = 0; c < size; ++c) {
            char ch = board[r][c];
            if (dead[r][c]) ch = static_cast<char>(std::tolower(ch));  // marked dead
            char sep = ' ';
            if (r == lr && c == lc) sep = '(';
            else if (r == lr && c == lc + 1) sep = ')';
            std::cout << sep << ch;
        }
        std::cout << (r == lr && lc == size - 1 ? ')' : ' ') << label << '\n';
    }
    std::cout << "   ";
    for (int c = 0; c < size; ++c) std::cout << COLUMNS[c] << ' ';
    std::cout << "\nCaptures - Noir (B): " << capturedByBlack
              << " | Blanc (W): " << capturedByWhite << "\n\n";
}

int GoGame::collectGroup(const std::vector<std::vector<char>>& b, int r, int c, Group& group) const {
    char color = b[r][c];
    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    std::set<std::pair<int, int>> liberties;
    std::vector<std::pair<int, int>> stack{{r, c}};
    seen[r][c] = true;
    while (!stack.empty()) {
        auto [cr, cc] = stack.back();
        stack.pop_back();
        group.push_back({cr, cc});
        for (int d = 0; d < 4; ++d) {
            int nr = cr + DR[d], nc = cc + DC[d];
            if (!inBoard(nr, nc)) continue;
            if (b[nr][nc] == EMPTY) {
                liberties.insert({nr, nc});
            } else if (b[nr][nc] == color && !seen[nr][nc]) {
                seen[nr][nc] = true;
                stack.push_back({nr, nc});
            }
        }
    }
    return static_cast<int>(liberties.size());
}

// Removes every enemy group adjacent to (r,c) that has no liberty left; returns stones removed.
int GoGame::removeDeadNeighbours(std::vector<std::vector<char>>& b, int r, int c, char enemy) const {
    int removed = 0;
    for (int d = 0; d < 4; ++d) {
        int nr = r + DR[d], nc = c + DC[d];
        if (!inBoard(nr, nc) || b[nr][nc] != enemy) continue;
        Group g;
        if (collectGroup(b, nr, nc, g) == 0) {
            for (auto& [gr, gc] : g) b[gr][gc] = EMPTY;
            removed += static_cast<int>(g.size());
        }
    }
    return removed;
}

std::string GoGame::serialize(const std::vector<std::vector<char>>& b) const {
    std::string s;
    for (const auto& row : b) s.append(row.begin(), row.end());
    return s;
}

MoveResult GoGame::placeStone(int row, int col, char player) {
    if (isOver()) return MoveResult::GameOver;
    if (!inBoard(row, col)) return MoveResult::OutOfBoard;
    if (board[row][col] != EMPTY) return MoveResult::Occupied;

    auto next = board;
    next[row][col] = player;
    int removed = removeDeadNeighbours(next, row, col, opponent(player));

    Group own;
    if (collectGroup(next, row, col, own) == 0) return MoveResult::Suicide;

    std::string key = serialize(next);
    if (history.count(key)) return MoveResult::Ko;

    saveSnapshot(player);
    moves.push_back({player, false, row, col});
    board = std::move(next);
    history.insert(key);
    (player == BLACK ? capturedByBlack : capturedByWhite) += removed;
    consecutivePasses = 0;
    return MoveResult::Ok;
}

void GoGame::pass(char player) {
    saveSnapshot(player);
    moves.push_back({player, true, -1, -1});
    ++consecutivePasses;
}

bool GoGame::toggleDead(int row, int col) {
    if (!inBoard(row, col) || board[row][col] == EMPTY) return false;
    Group g;
    collectGroup(board, row, col, g);
    bool mark = !dead[row][col];
    for (auto& [r, c] : g) dead[r][c] = mark;
    return true;
}

int GoGame::deadCount() const {
    int n = 0;
    for (const auto& row : dead) for (bool d : row) n += d;
    return n;
}

void GoGame::saveSnapshot(char mover) {
    undoStack.push_back({board, history, consecutivePasses, capturedByBlack, capturedByWhite, mover});
}

bool GoGame::undo(char& player) {
    if (undoStack.empty()) return false;
    Snapshot& s = undoStack.back();
    board = std::move(s.board);
    history = std::move(s.history);
    consecutivePasses = s.passes;
    capturedByBlack = s.capB;
    capturedByWhite = s.capW;
    player = s.mover;
    undoStack.pop_back();
    moves.pop_back();
    for (auto& row : dead) row.assign(size, false);
    return true;
}

int GoGame::deadCount(char player) const {
    int n = 0;
    for (int r = 0; r < size; ++r)
        for (int c = 0; c < size; ++c) n += (dead[r][c] && board[r][c] == player);
    return n;
}

void GoGame::resume() {
    consecutivePasses = 0;
    for (auto& row : dead) row.assign(size, false);
}

// For every empty (or dead-marked) cell: BLACK/WHITE if its region is bordered by live stones of
// that colour only, EMPTY if neutral. Live stones are marked ' '.
std::vector<std::vector<char>> GoGame::territoryMap() const {
    std::vector<std::vector<char>> owner(size, std::vector<char>(size, ' '));
    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if ((board[r][c] != EMPTY && !dead[r][c]) || seen[r][c]) continue;
            // Flood an empty region and see which colours border it.
            std::vector<std::pair<int, int>> region, stack{{r, c}};
            bool touchesB = false, touchesW = false;
            seen[r][c] = true;
            while (!stack.empty()) {
                auto [cr, cc] = stack.back();
                stack.pop_back();
                region.push_back({cr, cc});
                for (int d = 0; d < 4; ++d) {
                    int nr = cr + DR[d], nc = cc + DC[d];
                    if (!inBoard(nr, nc)) continue;
                    char v = dead[nr][nc] ? EMPTY : board[nr][nc];
                    if (v == BLACK) touchesB = true;
                    else if (v == WHITE) touchesW = true;
                    else if (!seen[nr][nc]) { seen[nr][nc] = true; stack.push_back({nr, nc}); }
                }
            }
            char who = (touchesB && !touchesW) ? BLACK : (touchesW && !touchesB) ? WHITE : EMPTY;
            for (auto& [rr, rc] : region) owner[rr][rc] = who;
        }
    }
    return owner;
}

void GoGame::computeScore(double& blackScore, double& whiteScore) const {
    int black = 0, white = 0;
    auto owner = territoryMap();
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (owner[r][c] == ' ') (board[r][c] == BLACK ? black : white) += 1;
            else if (owner[r][c] == BLACK) ++black;
            else if (owner[r][c] == WHITE) ++white;
        }
    }
    blackScore = black;
    whiteScore = white + komi;
}

// Board with territory shown as '+' (Black) and '-' (White); neutral points stay '.'.
void GoGame::printTerritory() const {
    auto owner = territoryMap();
    std::cout << "\n   ";
    for (int c = 0; c < size; ++c) std::cout << COLUMNS[c] << ' ';
    std::cout << '\n';
    int tb = 0, tw = 0;
    for (int r = 0; r < size; ++r) {
        int label = size - r;
        if (label < 10) std::cout << ' ';
        std::cout << label << ' ';
        for (int c = 0; c < size; ++c) {
            char ch;
            if (owner[r][c] == ' ') ch = board[r][c];
            else if (owner[r][c] == BLACK) { ch = '+'; ++tb; }
            else if (owner[r][c] == WHITE) { ch = '-'; ++tw; }
            else ch = '.';
            std::cout << ch << ' ';
        }
        std::cout << label << '\n';
    }
    std::cout << "   ";
    for (int c = 0; c < size; ++c) std::cout << COLUMNS[c] << ' ';
    std::cout << "\nTerritoire - Noir (+) : " << tb << " | Blanc (-) : " << tw << "\n\n";
}

std::string GoGame::describe(MoveResult r) {
    switch (r) {
        case MoveResult::Ok: return "Coup valide.";
        case MoveResult::OutOfBoard: return "Position hors du plateau.";
        case MoveResult::Occupied: return "Cette intersection est deja occupee.";
        case MoveResult::Suicide: return "Coup interdit : suicide (aucune liberte).";
        case MoveResult::Ko: return "Coup interdit : regle du ko (position deja rencontree).";
        case MoveResult::GameOver: return "La partie est terminee.";
    }
    return "";
}

std::vector<std::string> GoGame::stonePositions(char player) const {
    std::vector<std::string> list;
    for (int r = 0; r < size; ++r)
        for (int c = 0; c < size; ++c)
            if (board[r][c] == player) list.push_back(coordName(r, c));
    return list;
}

void GoGame::libertyStats(char player, int& groups, int& totalLiberties, int& weakest) const {
    groups = 0;
    totalLiberties = 0;
    weakest = 0;
    std::vector<std::vector<bool>> counted(size, std::vector<bool>(size, false));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (board[r][c] != player || counted[r][c]) continue;
            Group g;
            int libs = collectGroup(board, r, c, g);
            for (auto& [gr, gc] : g) counted[gr][gc] = true;
            ++groups;
            totalLiberties += libs;
            if (groups == 1 || libs < weakest) weakest = libs;
        }
    }
}

void GoGame::territoryStats(char player, int& regions, int& points) const {
    regions = 0;
    points = 0;
    auto owner = territoryMap();
    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (owner[r][c] != player || seen[r][c]) continue;
            ++regions;
            std::vector<std::pair<int, int>> stack{{r, c}};
            seen[r][c] = true;
            while (!stack.empty()) {
                auto [cr, cc] = stack.back();
                stack.pop_back();
                ++points;
                for (int d = 0; d < 4; ++d) {
                    int nr = cr + DR[d], nc = cc + DC[d];
                    if (inBoard(nr, nc) && owner[nr][nc] == player && !seen[nr][nc]) {
                        seen[nr][nc] = true;
                        stack.push_back({nr, nc});
                    }
                }
            }
        }
    }
}

void GoGame::groupStats(char player, int& groups, int& largest) const {
    groups = 0;
    largest = 0;
    std::vector<std::vector<bool>> counted(size, std::vector<bool>(size, false));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (board[r][c] != player || counted[r][c]) continue;
            Group g;
            collectGroup(board, r, c, g);
            for (auto& [gr, gc] : g) counted[gr][gc] = true;
            ++groups;
            if (static_cast<int>(g.size()) > largest) largest = static_cast<int>(g.size());
        }
    }
}

int GoGame::stonesPlayed(char player) const {
    int n = 0;
    for (const Move& m : moves) n += (m.player == player && !m.isPass);
    return n;
}

int GoGame::passesBy(char player) const {
    int n = 0;
    for (const Move& m : moves) n += (m.player == player && m.isPass);
    return n;
}

int GoGame::stonesOnBoard(char player) const {
    int n = 0;
    for (const auto& row : board) for (char ch : row) n += (ch == player);
    return n;
}

std::string GoGame::coordName(int row, int col) const {
    return std::string(1, COLUMNS[col]) + std::to_string(size - row);
}

bool GoGame::groupInfo(int row, int col, std::vector<std::string>& stones,
                       std::vector<std::string>& liberties) const {
    if (!inBoard(row, col) || board[row][col] == EMPTY) return false;
    Group g;
    collectGroup(board, row, col, g);
    std::set<std::pair<int, int>> libs;
    for (auto& [r, c] : g) {
        stones.push_back(coordName(r, c));
        for (int d = 0; d < 4; ++d) {
            int nr = r + DR[d], nc = c + DC[d];
            if (inBoard(nr, nc) && board[nr][nc] == EMPTY) libs.insert({nr, nc});
        }
    }
    for (auto& [r, c] : libs) liberties.push_back(coordName(r, c));
    return true;
}

std::vector<std::string> GoGame::moveList() const {
    std::vector<std::string> list;
    for (const Move& m : moves) {
        std::string s(1, m.player);
        s += ' ';
        if (m.isPass) s += "pass";
        else s += coordName(m.row, m.col);
        list.push_back(s);
    }
    return list;
}

bool GoGame::saveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;
    out << "GOGAME 1\n" << size << ' ' << komi << '\n';
    for (const Move& m : moves) {
        out << m.player << ' ';
        if (m.isPass) out << "pass\n";
        else out << m.row << ' ' << m.col << '\n';
    }
    return static_cast<bool>(out);
}

bool GoGame::loadFromFile(const std::string& path, char& player, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "Impossible d'ouvrir le fichier."; return false; }
    std::string magic;
    int version = 0, newSize = 0;
    double newKomi = 0;
    if (!(in >> magic >> version >> newSize >> newKomi) || magic != "GOGAME" || version != 1 ||
        newSize < 5 || newSize > 19) {
        error = "Fichier de sauvegarde invalide.";
        return false;
    }
    GoGame loaded(newSize, newKomi);
    char next = BLACK;
    char who;
    std::string a;
    while (in >> who >> a) {
        if (who != BLACK && who != WHITE) { error = "Fichier corrompu (joueur inconnu)."; return false; }
        if (a == "pass") {
            loaded.pass(who);
        } else {
            int row, col;
            try { row = std::stoi(a); } catch (...) { error = "Fichier corrompu."; return false; }
            if (!(in >> col) || loaded.placeStone(row, col, who) != MoveResult::Ok) {
                error = "Fichier corrompu (coup illegal).";
                return false;
            }
        }
        next = opponent(who);
    }
    *this = std::move(loaded);
    player = next;
    return true;
}
