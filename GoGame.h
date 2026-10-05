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

    // Dead-stone marking (after both players passed): toggles the whole group at (row,col).
    bool toggleDead(int row, int col);
    int deadCount() const;
    // Cancels the end of the game: clears dead marks and the pass counter.
    void resume();

    // Undoes the last move or pass; sets `player` to whoever played it. False if nothing to undo.
    bool undo(char& player);

    // Saves the game as its move list (so undo and ko history survive a reload).
    bool saveToFile(const std::string& path) const;
    // Replaces this game by the one stored in `path`; `player` becomes the side to move.
    // On failure the game is left untouched and `error` explains why.
    bool loadFromFile(const std::string& path, char& player, std::string& error);

    // One entry per move, e.g. "B D4" or "W pass" (columns A-T without I, row 1 at the bottom).
    std::vector<std::string> moveList() const;

    int captures(char player) const { return player == BLACK ? capturedByBlack : capturedByWhite; }
    // Area scoring (stones + surrounded territory), komi added to White.
    void computeScore(double& blackScore, double& whiteScore) const;

    double getKomi() const { return komi; }
    void setKomi(double k) { komi = k; }
    int getSize() const { return size; }
    static char opponent(char player) { return player == BLACK ? WHITE : BLACK; }
    static std::string describe(MoveResult r);

private:
    int size;
    double komi;
    std::vector<std::vector<char>> board;
    std::set<std::string> history;  // positional superko
    struct Snapshot {
        std::vector<std::vector<char>> board;
        std::set<std::string> history;
        int passes, capB, capW;
        char mover;
    };
    std::vector<Snapshot> undoStack;
    struct Move { char player; bool isPass; int row, col; };
    std::vector<Move> moves;
    void saveSnapshot(char mover);
    std::vector<std::vector<bool>> dead;
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
