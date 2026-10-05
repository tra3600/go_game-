#ifndef GOGAME_H
#define GOGAME_H

#include <set>
#include <string>
#include <utility>
#include <vector>

enum class MoveResult {
    Ok,
    OutOfBoard,
    Occupied,
    Suicide,
    Ko,
    GameOver
};

class GoGame {
public:
    static constexpr char EMPTY = '.';
    static constexpr char BLACK = 'B';
    static constexpr char WHITE = 'W';

    explicit GoGame(int size, double komi = 6.5);

    void printBoard() const;
    MoveResult placeStone(int row, int col, char player);
    void pass(char player);
    bool isOver() const { return consecutivePasses >= 2; }

    int captures(char player) const { return player == BLACK ? capturedByBlack : capturedByWhite; }
    // Area scoring (stones + surrounded territory), komi added to White.
    void computeScore(double& blackScore, double& whiteScore) const;

    int getSize() const { return size; }
    static char opponent(char player) { return player == BLACK ? WHITE : BLACK; }
    static std::string describe(MoveResult r);

private:
    int size;
    double komi;
    std::vector<std::vector<char>> board;
    std::set<std::string> history;  // positional superko
    int consecutivePasses = 0;
    int capturedByBlack = 0;
    int capturedByWhite = 0;

    using Group = std::vector<std::pair<int, int>>;
    bool inBoard(int r, int c) const { return r >= 0 && r < size && c >= 0 && c < size; }
    // Collects the group containing (r,c) and returns its number of liberties.
    int collectGroup(const std::vector<std::vector<char>>& b, int r, int c, Group& group) const;
    int removeDeadNeighbours(std::vector<std::vector<char>>& b, int r, int c, char enemy) const;
    std::string serialize(const std::vector<std::vector<char>>& b) const;
};

#endif // GOGAME_H
