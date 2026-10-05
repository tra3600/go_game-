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

// Splits "commande argument" : returns the lowercase command, `arg` receives the rest (case preserved).
std::string splitCommand(const std::string& line, std::string& arg) {
    std::istringstream in(line);
    std::string word;
    in >> word;
    std::getline(in >> std::ws, arg);
    for (char& ch : word) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return word;
}

void printScore(const GoGame& game) {
    double b, w;
    game.computeScore(b, w);
    std::cout << "Score actuel (aire + komi) - Noir : " << b << " | Blanc : " << w << "\n"
              << (b > w ? "Noir mene de " : "Blanc mene de ") << (b > w ? b - w : w - b) << " points.\n"
              << "(Estimation : les pierres non marquees mortes comptent comme vivantes.)\n";
}

void printHistory(const GoGame& game) {
    auto list = game.moveList();
    if (list.empty()) {
        std::cout << "Aucun coup joue.\n";
        return;
    }
    std::cout << "Historique (" << list.size() << " coups) :\n";
    for (size_t i = 0; i < list.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << list[i] << "\n";
    }
}

void printLastMove(const GoGame& game) {
    auto list = game.moveList();
    if (list.empty()) std::cout << "Aucun coup joue.\n";
    else std::cout << "Dernier coup (n." << list.size() << ") : " << list.back() << "\n";
}

void printHelp() {
    std::cout << "\nCommandes :\n"
              << "  D4 | ligne colonne    jouer une pierre (ex: D4 ou 4 4)\n"
              << "  pass | p              passer (deux passes de suite = fin de partie)\n"
              << "  annuler | undo | u    annuler le dernier coup ou la derniere passe\n"
              << "  sauvegarder [fichier] sauvegarder la partie (defaut : partie.go)\n"
              << "  charger [fichier]     charger une partie sauvegardee\n"
              << "  score                 afficher le score en cours (estimation)\n"
              << "  historique | histo    afficher la liste des coups joues\n"
              << "  dernier | last        afficher le dernier coup joue\n"
              << "  aide | help | ?       afficher cette aide\n"
              << "  quit | q              quitter\n"
              << "Phase de marquage : coordonnee = marquer/demarquer un groupe mort, 'ok' = valider le score,\n"
              << "  'reprendre' = continuer la partie, 'annuler' = annuler la derniere passe,\n"
              << "  'score', 'historique', 'dernier', 'sauvegarder' et 'aide' restent disponibles.\n\n";
}

void saveCommand(const GoGame& game, const std::string& arg) {
    std::string path = arg.empty() ? "partie.go" : arg;
    if (game.saveToFile(path)) std::cout << "Partie sauvegardee dans " << path << ".\n";
    else std::cout << "Echec de la sauvegarde dans " << path << ".\n";
}
}

int main(int argc, char** argv) {
    int size = 0;
    char player = GoGame::BLACK;
    GoGame game(5);
    if (argc > 1) {
        std::string err;
        if (!game.loadFromFile(argv[1], player, err)) {
            std::cout << err << "\n";
            return 1;
        }
        size = game.getSize();
        std::cout << "Partie chargee depuis " << argv[1] << ".\n";
    } else {
        size = readBoardSize();
        if (size < 0) return 0;
        game = GoGame(size);
    }
    std::cout << "Tapez 'aide' pour voir les commandes.\n";

    bool finished = false;
    while (!finished) {
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
        std::string arg;
        std::string word = splitCommand(line, arg);
        if (word == "historique" || word == "histo" || word == "history") {
            printHistory(game);
            continue;
        }
        if (word == "dernier" || word == "last") {
            printLastMove(game);
            continue;
        }
        if (word == "score") {
            printScore(game);
            continue;
        }
        if (word == "aide" || word == "help" || word == "?" || word == "h") {
            printHelp();
            continue;
        }
        if (word == "sauvegarder" || word == "save") {
            saveCommand(game, arg);
            continue;
        }
        if (word == "charger" || word == "load") {
            std::string err;
            std::string path = arg.empty() ? "partie.go" : arg;
            if (game.loadFromFile(path, player, err)) {
                size = game.getSize();
                std::cout << "Partie chargee depuis " << path << ".\n";
            } else {
                std::cout << err << "\n";
            }
            continue;
        }
        if (cmd == "undo" || cmd == "annuler" || cmd == "u") {
            if (game.undo(player)) std::cout << "Dernier coup annule.\n";
            else std::cout << "Rien a annuler.\n";
            continue;
        }
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

    bool resumed = false;
    if (game.isOver()) {
        std::cout << "\nMarquage des pierres mortes : entrez une pierre (ex: D4) pour marquer/demarquer\n"
                  << "son groupe (affiche en minuscule), 'ok' pour valider le score,\n"
                  << "'reprendre' pour continuer la partie (desaccord).\n";
        while (true) {
            game.printBoard();
            std::cout << "Mort > ";
            std::string line;
            if (!std::getline(std::cin, line)) break;
            std::string cmd;
            for (char ch : line) cmd += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            while (!cmd.empty() && std::isspace(static_cast<unsigned char>(cmd.back()))) cmd.pop_back();
            if (cmd == "ok" || cmd == "done") break;
            std::string arg;
            std::string word = splitCommand(line, arg);
            if (word == "historique" || word == "histo" || word == "history") {
                printHistory(game);
                continue;
            }
            if (word == "dernier" || word == "last") {
                printLastMove(game);
                continue;
            }
            if (word == "score") {
                printScore(game);
                continue;
            }
            if (word == "aide" || word == "help" || word == "?" || word == "h") {
                printHelp();
                continue;
            }
            if (word == "sauvegarder" || word == "save") {
                saveCommand(game, arg);
                continue;
            }
            if (cmd == "quit" || cmd == "q") return 0;
            if (cmd == "undo" || cmd == "annuler" || cmd == "u") {
                game.undo(player);
                resumed = true;
                std::cout << "Derniere passe annulee. Joueur " << player << " a la main.\n";
                break;
            }
            if (cmd == "reprendre" || cmd == "resume") {
                game.resume();
                resumed = true;
                std::cout << "La partie reprend. Joueur " << player << " a la main.\n";
                break;
            }
            int row, col;
            if (!parseMove(line, size, row, col) || !game.toggleDead(row, col))
                std::cout << "Choisissez une pierre presente sur le plateau.\n";
        }
    }
    finished = !resumed;
    }

    game.printBoard();
    double b, w;
    game.computeScore(b, w);
    std::cout << "Partie terminee. Score (aire + komi 6.5) - Noir : " << b << " | Blanc : " << w << "\n";
    std::cout << (b > w ? "Noir gagne" : "Blanc gagne") << " de " << (b > w ? b - w : w - b) << " points.\n";
    return 0;
}
