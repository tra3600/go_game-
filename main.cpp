#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include "GoGame.h"

namespace {
const std::string COLUMNS = "ABCDEFGHJKLMNOPQRST";

// Parses "D4" (column letter + row number counted from the bottom) or "x y" (row col, 1-based from top-left).
bool parseMove(const std::string& line, int size, int& row, int& col) {
    std::istringstream in(line);
    std::string first;
    if (!(in >> first)) return false;

    if (std::isalpha(static_cast<unsigned char>(first[0]))) {
        size_t pos = COLUMNS.find(static_cast<char>(std::toupper(static_cast<unsigned char>(first[0]))));
        if (pos == std::string::npos || pos >= static_cast<size_t>(size)) return false;
        std::string num = first.substr(1);
        if (num.empty()) in >> num;
        try {
            size_t used = 0;
            int n = std::stoi(num, &used);
            if (used != num.size()) return false;
            row = size - n;
        } catch (...) { return false; }
        col = static_cast<int>(pos);
        return row >= 0 && row < size;
    }

    try {
        int a = std::stoi(first), b;
        if (!(in >> b)) return false;
        row = a - 1;
        col = b - 1;
    } catch (...) { return false; }
    return true;
}

int readBoardSize() {
    while (true) {
        std::cout << "Taille du plateau (5 a 19, ex: 19) : ";
        std::string line;
        if (!std::getline(std::cin, line)) return -1;
        try {
            int s = std::stoi(line);
            if (s >= 5 && s <= 19) return s;
        } catch (...) {}
        std::cout << "Taille invalide.\n";
    }
}
}

int main() {
    int size = readBoardSize();
    if (size < 0) return 0;

    GoGame game(size);
    char player = GoGame::BLACK;
    std::cout << "Commandes : D4 (colonne+ligne) ou \"ligne colonne\", 'pass' pour passer, 'quit' pour quitter.\n"
              << "Deux passes consecutives terminent la partie.\n";

    while (!game.isOver()) {
        game.printBoard();
        std::cout << "Joueur " << player << " (" << (player == GoGame::BLACK ? "Noir" : "Blanc") << ") > ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        std::string cmd;
        for (char ch : line) cmd += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        while (!cmd.empty() && std::isspace(static_cast<unsigned char>(cmd.back()))) cmd.pop_back();
        if (cmd.empty()) continue;
        if (cmd == "quit" || cmd == "q") return 0;
        if (cmd == "pass" || cmd == "p") {
            game.pass(player);
            player = GoGame::opponent(player);
            continue;
        }

        int row, col;
        if (!parseMove(line, size, row, col)) {
            std::cout << "Entree invalide. Exemple : D4 ou 4 4.\n";
            continue;
        }
        MoveResult r = game.placeStone(row, col, player);
        if (r != MoveResult::Ok) {
            std::cout << GoGame::describe(r) << " Reessayez.\n";
            continue;
        }
        player = GoGame::opponent(player);
    }

    if (game.isOver()) {
        std::cout << "\nMarquage des pierres mortes : entrez une pierre (ex: D4) pour marquer/demarquer\n"
                  << "son groupe (affiche en minuscule), puis 'ok' pour valider le score.\n";
        while (true) {
            game.printBoard();
            std::cout << "Mort > ";
            std::string line;
            if (!std::getline(std::cin, line)) break;
            std::string cmd;
            for (char ch : line) cmd += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            while (!cmd.empty() && std::isspace(static_cast<unsigned char>(cmd.back()))) cmd.pop_back();
            if (cmd == "ok" || cmd == "done") break;
            if (cmd == "quit" || cmd == "q") return 0;
            int row, col;
            if (!parseMove(line, size, row, col) || !game.toggleDead(row, col))
                std::cout << "Choisissez une pierre presente sur le plateau.\n";
        }
    }

    game.printBoard();
    double b, w;
    game.computeScore(b, w);
    std::cout << "Partie terminee. Score (aire + komi 6.5) - Noir : " << b << " | Blanc : " << w << "\n";
    std::cout << (b > w ? "Noir gagne" : "Blanc gagne") << " de " << (b > w ? b - w : w - b) << " points.\n";
    return 0;
}
