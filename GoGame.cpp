#include "GoGame.h"
#include <iostream>
#include <queue>

GoGame::GoGame(int size) : size(size), board(size, std::vector<char>(size, '.')) {}

void GoGame::printBoard() const {
    for (const auto& row : board) {
        for (char cell : row) {
            std::cout << cell << " ";
        }
        std::cout << std::endl;
    }
}

bool GoGame::placeStone(int x, int y, char player) {
    if (x < 0 || x >= size || y < 0 || y >= size || board[x][y] != '.') {
        return false;
    }
    board[x][y] = player;
    removeCapturedStones(player == 'B' ? 'W' : 'B');
    return true;
}

bool GoGame::isCaptured(int x, int y, char player) {
    if (x < 0 || x >= size || y < 0 || y >= size || board[x][y] != player) {
        return false;
    }

    std::vector<std::vector<bool>> visited(size, std::vector<bool>(size, false));
    bool captured = true;
    floodFill(x, y, player, visited, captured);
    return captured;
}

void GoGame::removeCapturedStones(char player) {
    for (int x = 0; x < size; ++x) {
        for (int y = 0; y < size; ++y) {
            if (board[x][y] == player && isCaptured(x, y, player)) {
                board[x][y] = '.';
            }
        }
    }
}

void GoGame::floodFill(int x, int y, char player, std::vector<std::vector<bool>>& visited, bool& isCaptured) {
    if (x < 0 || x >= size || y < 0 || y >= size || visited[x][y] || board[x][y] != player) {
        return;
    }

    visited[x][y] = true;

    if (x > 0 && board[x - 1][y] == '.') {
        isCaptured = false;
    }
    if (x < size - 1 && board[x + 1][y] == '.') {
        isCaptured = false;
    }
    if (y > 0 && board[x][y - 1] == '.') {
        isCaptured = false;
    }
    if (y < size - 1 && board[x][y + 1] == '.') {
        isCaptured = false;
    }

    floodFill(x - 1, y, player, visited, isCaptured);
    floodFill(x + 1, y, player, visited, isCaptured);
    floodFill(x, y - 1, player, visited, isCaptured);
    floodFill(x, y + 1, player, visited, isCaptured);
}