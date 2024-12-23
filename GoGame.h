#ifndef GOGAME_H
#define GOGAME_H

#include <vector>
#include <string>

class GoGame {
public:
    GoGame(int size);
    void printBoard() const;
    bool placeStone(int x, int y, char player);
    bool isCaptured(int x, int y, char player);
    void removeCapturedStones(char player);

private:
    int size;
    std::vector<std::vector<char>> board;
    void floodFill(int x, int y, char player, std::vector<std::vector<bool>>& visited, bool& isCaptured);
};

#endif // GOGAME_H