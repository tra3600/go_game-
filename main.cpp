#include <iostream>
#include "GoGame.h"

int main() {
    int size;
    std::cout << "Enter board size (e.g., 19 for a 19x19 board): ";
    std::cin >> size;

    GoGame game(size);
    char player = 'B';  // Black starts
    int x, y;

    while (true) {
        game.printBoard();
        std::cout << "Player " << player << ", enter your move (x y): ";
        std::cin >> x >> y;

        if (!game.placeStone(x, y, player)) {
            std::cout << "Invalid move. Try again." << std::endl;
            continue;
        }

        player = (player == 'B') ? 'W' : 'B';  // Switch players
    }

    return 0;
}