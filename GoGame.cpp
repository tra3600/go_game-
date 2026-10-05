#include "GoGame.h"
#include <cctype>
#include <iostream>

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

void GoGame::printBoard() const {
    std::cout << "\n   ";
    for (int c = 0; c < size; ++c) std::cout << COLUMNS[c] << ' ';
    std::cout << '\n';
    for (int r = 0; r < size; ++r) {
        int label = size - r;
        if (label < 10) std::cout << ' ';
        std::cout << label << ' ';
        for (int c = 0; c < size; ++c) {
            char ch = board[r][c];
            if (dead[r][c]) ch = static_cast<char>(std::tolower(ch));  // marked dead
            std::cout << ch << ' ';
        }
        std::cout << label << '\n';
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
    board = std::move(next);
    history.insert(key);
    (player == BLACK ? capturedByBlack : capturedByWhite) += removed;
    consecutivePasses = 0;
    return MoveResult::Ok;
}

void GoGame::pass(char player) {
    saveSnapshot(player);
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
    for (auto& row : dead) row.assign(size, false);
    return true;
}

void GoGame::resume() {
    consecutivePasses = 0;
    for (auto& row : dead) row.assign(size, false);
}

void GoGame::computeScore(double& blackScore, double& whiteScore) const {
    int black = 0, white = 0;
    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            if (board[r][c] != EMPTY && !dead[r][c]) {
                (board[r][c] == BLACK ? black : white) += 1;
                continue;
            }
            if (seen[r][c]) continue;
            // Flood an empty region and see which colours border it.
            int area = 0;
            bool touchesB = false, touchesW = false;
            std::vector<std::pair<int, int>> stack{{r, c}};
            seen[r][c] = true;
            while (!stack.empty()) {
                auto [cr, cc] = stack.back();
                stack.pop_back();
                ++area;
                for (int d = 0; d < 4; ++d) {
                    int nr = cr + DR[d], nc = cc + DC[d];
                    if (!inBoard(nr, nc)) continue;
                    char v = dead[nr][nc] ? EMPTY : board[nr][nc];
                    if (v == BLACK) touchesB = true;
                    else if (v == WHITE) touchesW = true;
                    else if (!seen[nr][nc]) { seen[nr][nc] = true; stack.push_back({nr, nc}); }
                }
            }
            if (touchesB && !touchesW) black += area;
            else if (touchesW && !touchesB) white += area;
        }
    }
    blackScore = black;
    whiteScore = white + komi;
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
