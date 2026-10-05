#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include "GoGame.h"

namespace {
const std::string COLUMNS = "ABCDEFGHJKLMNOPQRST";

// Play time: the time between two moves is credited to the player who was to move.
// Not stored in save files, so it restarts on load / new game.
struct GameClock {
    using Clock = std::chrono::steady_clock;
    Clock::time_point start = Clock::now();
    Clock::time_point lastTick = start;
    std::time_t startWall = std::time(nullptr);  // calendar time, for display only
    double used[2] = {0, 0};  // seconds spent by Black, White

    static int index(char player) { return player == GoGame::BLACK ? 0 : 1; }
    void reset() { *this = GameClock(); }
    void pause() { lastTick = Clock::now(); }  // after undo / resume: the elapsed gap is credited to nobody
    void moveDone(char player) {
        auto now = Clock::now();
        used[index(player)] += std::chrono::duration<double>(now - lastTick).count();
        lastTick = now;
    }
    double pending() const { return std::chrono::duration<double>(Clock::now() - lastTick).count(); }
    double total() const { return std::chrono::duration<double>(Clock::now() - start).count(); }
};
GameClock gameClock;

// Player names (default Noir / Blanc). Not stored in save files.
std::string playerNames[2] = {"Noir", "Blanc"};
const std::string& nameOf(char player) { return playerNames[player == GoGame::BLACK ? 0 : 1]; }

std::string formatDuration(double seconds) {
    long s = static_cast<long>(seconds);
    char buf[32];
    if (s >= 3600) std::snprintf(buf, sizeof buf, "%ldh %02ldmin %02lds", s / 3600, (s % 3600) / 60, s % 60);
    else if (s >= 60) std::snprintf(buf, sizeof buf, "%ldmin %02lds", s / 60, s % 60);
    else std::snprintf(buf, sizeof buf, "%lds", s);
    return buf;
}

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

const char* const GAME_VERSION = "1.0.0";

void printVersion() {
    std::cout << "Jeu de Go (C++) version " << GAME_VERSION << " - format de sauvegarde 1\n";
}

void printPlayTime(char player, bool over) {
    double b = gameClock.used[0], w = gameClock.used[1];
    if (!over) (player == GoGame::BLACK ? b : w) += gameClock.pending();
    std::cout << "\nTemps de jeu\n"
              << "  Duree totale : " << formatDuration(gameClock.total()) << "\n"
              << "  Temps de reflexion - Noir : " << formatDuration(b) << " | Blanc : " << formatDuration(w) << "\n"
              << "  (compte depuis le debut de la partie ou son chargement)\n\n";
}

void printPlayers() {
    std::cout << "Joueurs :\n"
              << "  Noir  (B) : " << playerNames[0] << "\n"
              << "  Blanc (W) : " << playerNames[1] << "\n"
              << "Pour changer un nom : nom noir <nom> | nom blanc <nom>\n";
}

void nameCommand(const std::string& arg) {
    std::istringstream in(arg);
    std::string side, name;
    in >> side;
    std::getline(in >> std::ws, name);
    for (char& ch : side) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    int idx = (side == "noir" || side == "b") ? 0 : (side == "blanc" || side == "w") ? 1 : -1;
    if (idx < 0 || name.empty() || name.size() > 20) {
        std::cout << "Usage : nom noir <nom> | nom blanc <nom> (nom de 1 a 20 caracteres).\n";
        return;
    }
    playerNames[idx] = name;
    std::cout << (idx == 0 ? "Noir" : "Blanc") << " (" << (idx == 0 ? 'B' : 'W') << ") s'appelle maintenant " << name << ".\n";
}

std::string formatDate(std::time_t t) {
    static const char* const DAYS[] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
    std::tm tmv = *std::localtime(&t);
    char buf[48];
    std::snprintf(buf, sizeof buf, "%s %02d/%02d/%04d a %02d:%02d", DAYS[tmv.tm_wday], tmv.tm_mday,
                  tmv.tm_mon + 1, tmv.tm_year + 1900, tmv.tm_hour, tmv.tm_min);
    return buf;
}

void printDate() {
    std::cout << "Date et heure : " << formatDate(std::time(nullptr)) << "\n"
              << "Partie commencee : " << formatDate(gameClock.startWall) << "\n";
}

void printTime() {
    std::time_t t = std::time(nullptr);
    std::tm tmv = *std::localtime(&t);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    std::cout << "Il est " << buf << ".\n";
}

void printTimezone() {
    std::time_t t = std::time(nullptr);
    std::tm tmv = *std::localtime(&t);
    char abbr[16] = "", off[16] = "";
    std::strftime(abbr, sizeof abbr, "%Z", &tmv);
    std::strftime(off, sizeof off, "%z", &tmv);  // +hhmm
    std::string o = off;
    if (o.size() == 5) o = o.substr(0, 3) + ":" + o.substr(3);
    std::cout << "Fuseau horaire : " << abbr << " (UTC" << o << ")"
              << (tmv.tm_isdst > 0 ? ", heure d'ete" : "") << "\n";
}

void printLanguage() {
    std::cout << "Langue du jeu : francais (seule langue disponible pour l'instant).\n";
}

void printMoveCount(const GoGame& game) {
    int total = static_cast<int>(game.moveList().size());
    int bs = game.stonesPlayed(GoGame::BLACK), ws = game.stonesPlayed(GoGame::WHITE);
    int bp = game.passesBy(GoGame::BLACK), wp = game.passesBy(GoGame::WHITE);
    std::cout << "Nombre de coups : " << total << " (" << bs + ws << " pierre(s) posee(s), " << bp + wp << " passe(s))\n"
              << "  " << nameOf(GoGame::BLACK) << " (B) : " << bs + bp << " (" << bs << " pose(s), " << bp << " passe(s))\n"
              << "  " << nameOf(GoGame::WHITE) << " (W) : " << ws + wp << " (" << ws << " pose(s), " << wp << " passe(s))\n";
}

void printStoneCount(const GoGame& game) {
    int b = game.stonesOnBoard(GoGame::BLACK), w = game.stonesOnBoard(GoGame::WHITE);
    int cells = game.getSize() * game.getSize();
    std::cout << "Pierres sur le plateau : " << b + w << "\n"
              << "  " << nameOf(GoGame::BLACK) << " (B) : " << b << "\n"
              << "  " << nameOf(GoGame::WHITE) << " (W) : " << w << "\n"
              << "Intersections libres : " << cells - b - w << " sur " << cells << "\n";
    if (game.deadCount() > 0) std::cout << "(dont " << game.deadCount() << " pierre(s) marquee(s) morte(s))\n";
}

void printCredits() {
    std::cout << "\nCredits\n"
              << "  Jeu de Go en C++ - version " << GAME_VERSION << "\n"
              << "  Projet : tra3600/go_game- (https://github.com/tra3600/go_game-)\n"
              << "  Ameliorations (captures, ko, marquage des pierres mortes, commandes...) : Claude Code\n"
              << "  Licence : GNU LGPL 2.1 (voir le fichier LICENSE)\n"
              << "  Le jeu de Go est un jeu traditionnel d'origine asiatique, vieux de plus de 2500 ans.\n\n";
}

// Two-letter shortcuts (never valid coordinates, so they cannot clash with "D4"-style moves).
struct Shortcut { const char* alias; const char* command; };
const Shortcut SHORTCUTS[] = {
    {"sc", "score"},       {"st", "stats"},        {"de", "dernier"},   {"mo", "montrer"},
    {"ca", "captures"},    {"te", "territoire"},   {"li", "libertes"},  {"re", "regles"},
    {"hi", "historique"},  {"pi", "pierres"},      {"sv", "sauvegarder"}, {"ch", "charger"},
    {"ta", "taille"},      {"ai", "aide"},        {"ve", "version"},
    {"cr", "credits"},
    {"da", "date"},
    {"he", "heure"},
    {"np", "nbpierres"},
    {"co", "coups"},
    {"la", "langue"},
    {"fu", "fuseau"},
    {"jo", "joueurs"},   {"no", "nom"},
    {"tp", "temps"},
};

// Splits "commande argument" : returns the lowercase command (shortcuts expanded),
// `arg` receives the rest (case preserved).
std::string splitCommand(const std::string& line, std::string& arg) {
    std::istringstream in(line);
    std::string word;
    in >> word;
    std::getline(in >> std::ws, arg);
    for (char& ch : word) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    for (const Shortcut& s : SHORTCUTS)
        if (word == s.alias) return s.command;
    return word;
}

void printShortcuts() {
    std::cout << "\nRaccourcis :\n"
              << "  p  = pass       u  = annuler      q  = quit       h ou ?  = aide\n";
    for (const Shortcut& s : SHORTCUTS) std::cout << "  " << s.alias << " = " << s.command << "\n";
    std::cout << "\n";
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

void komiCommand(GoGame& game, std::string arg) {
    if (arg.empty()) {
        std::cout << "Komi actuel : " << game.getKomi() << "\n";
        return;
    }
    for (char& ch : arg) if (ch == ',') ch = '.';
    try {
        size_t used = 0;
        double k = std::stod(arg, &used);
        if (used == arg.size() && k >= -50 && k <= 50) {
            game.setKomi(k);
            std::cout << "Komi fixe a " << k << ".\n";
            return;
        }
    } catch (...) {}
    std::cout << "Komi invalide (nombre entre -50 et 50, ex: komi 7.5).\n";
}

void showLastMove(const GoGame& game) {
    if (game.moveList().empty()) {
        std::cout << "Aucun coup joue.\n";
        return;
    }
    game.printBoard(true);
    if (game.hasLastStone()) printLastMove(game);
    else std::cout << "Le dernier coup est une passe (aucune pierre a montrer).\n";
}

// Starts a fresh game on another board size (komi kept). Asks for confirmation if a game is in progress.
void sizeCommand(GoGame& game, char& player, int& size, const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Taille actuelle : " << size << "x" << size << "\n";
        return;
    }
    int n = 0;
    try {
        size_t used = 0;
        n = std::stoi(arg, &used);
        if (used != arg.size()) n = 0;
    } catch (...) {}
    if (n < 5 || n > 19) {
        std::cout << "Taille invalide (entre 5 et 19, ex: taille 13).\n";
        return;
    }
    if (!game.moveList().empty()) {
        std::cout << "Cela efface la partie en cours. Confirmer (o/n) ? ";
        std::string answer;
        if (!std::getline(std::cin, answer) || (answer != "o" && answer != "O" && answer != "oui")) {
            std::cout << "Changement annule.\n";
            return;
        }
    }
    game = GoGame(n, game.getKomi());
    player = GoGame::BLACK;
    gameClock.reset();
    size = n;
    std::cout << "Nouvelle partie sur un plateau " << n << "x" << n << ".\n";
}

void printCaptures(const GoGame& game) {
    int b = game.captures(GoGame::BLACK), w = game.captures(GoGame::WHITE);
    std::cout << "Pierres capturees :\n"
              << "  Noir (B) a capture " << b << " pierre" << (b > 1 ? "s" : "") << " blanche" << (b > 1 ? "s" : "") << "\n"
              << "  Blanc (W) a capture " << w << " pierre" << (w > 1 ? "s" : "") << " noire" << (w > 1 ? "s" : "") << "\n";
}

void libertiesCommand(const GoGame& game, const std::string& arg) {
    int row, col;
    if (arg.empty() || !parseMove(arg, game.getSize(), row, col)) {
        std::cout << "Usage : libertes D4 (coordonnee d'une pierre du groupe).\n";
        return;
    }
    std::vector<std::string> stones, libs;
    if (!game.groupInfo(row, col, stones, libs)) {
        std::cout << "Aucune pierre a cette intersection.\n";
        return;
    }
    auto join = [](const std::vector<std::string>& v) {
        std::string s;
        for (const auto& x : v) s += (s.empty() ? "" : " ") + x;
        return s;
    };
    std::cout << "Groupe de " << stones.size() << " pierre" << (stones.size() > 1 ? "s" : "") << " : " << join(stones) << "\n"
              << libs.size() << " liberte" << (libs.size() > 1 ? "s" : "") << " : " << join(libs)
              << (libs.size() == 1 ? "  (atari !)" : "") << "\n";
}

void printStats(const GoGame& game, char player) {
    const char colors[2] = {GoGame::BLACK, GoGame::WHITE};
    const char* names[2] = {"Noir (B)", "Blanc (W)"};
    int total = static_cast<int>(game.moveList().size());
    std::cout << "\nStatistiques de la partie\n"
              << "  Plateau : " << game.getSize() << "x" << game.getSize() << ", komi " << game.getKomi() << "\n"
              << "  Coups joues : " << total << (game.isOver() ? " (partie terminee)" : "") << "\n";
    if (!game.isOver()) std::cout << "  Trait : " << nameOf(player) << "\n";
    for (int i = 0; i < 2; ++i) {
        char p = colors[i];
        std::cout << "  " << names[i] << " : " << game.stonesPlayed(p) << " pose(s), " << game.passesBy(p)
                  << " passe(s), " << game.stonesOnBoard(p) << " sur le plateau, "
                  << game.captures(p) << " capture(s)\n";
    }
    double b, w;
    game.computeScore(b, w);
    std::cout << "  Score estime : Noir " << b << " | Blanc " << w << "\n\n";
}

void printStones(const GoGame& game, std::string arg) {
    for (char& ch : arg) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    bool wantB = true, wantW = true;
    if (arg == "noir" || arg == "b" || arg == "black") wantW = false;
    else if (arg == "blanc" || arg == "w" || arg == "white") wantB = false;
    else if (!arg.empty()) {
        std::cout << "Usage : pierres [noir|blanc].\n";
        return;
    }
    auto show = [&](char p, const char* name) {
        auto list = game.stonePositions(p);
        std::cout << name << " (" << list.size() << ") :";
        if (list.empty()) std::cout << " aucune";
        for (const auto& s : list) std::cout << ' ' << s;
        std::cout << "\n";
    };
    if (wantB) show(GoGame::BLACK, "Noir (B)");
    if (wantW) show(GoGame::WHITE, "Blanc (W)");
}

void printRules() {
    std::cout << "\nRegles du jeu de Go (version implementee)\n"
              << "  But : controler plus de plateau que l'adversaire (pierres vivantes + territoire).\n"
              << "  1. Noir joue en premier, puis les joueurs posent une pierre chacun leur tour\n"
              << "     sur une intersection libre. Une pierre ne bouge plus une fois posee.\n"
              << "  2. Liberte : intersection vide adjacente (haut, bas, gauche, droite) a un groupe.\n"
              << "     Des pierres de meme couleur adjacentes forment un groupe.\n"
              << "  3. Capture : un groupe qui n'a plus aucune liberte est retire du plateau\n"
              << "     (les pierres retirees sont comptees comme captures).\n"
              << "  4. Suicide interdit : on ne peut pas jouer une pierre qui laisse son propre groupe\n"
              << "     sans liberte, sauf si ce coup capture des pierres adverses.\n"
              << "  5. Ko : un coup qui recree une position deja apparue pendant la partie est interdit.\n"
              << "  6. On peut passer. Deux passes consecutives terminent la partie.\n"
              << "  7. Fin : les joueurs marquent les pierres mortes, puis le score est calcule par aire :\n"
              << "     pierres vivantes + territoire (zones vides entourees par une seule couleur).\n"
              << "  8. Komi : points ajoutes a Blanc pour compenser l'avantage de jouer en premier\n"
              << "     (6.5 par defaut, modifiable avec la commande 'komi').\n\n";
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
              << "  komi [valeur]         afficher ou changer le komi (ex: komi 7.5)\n"
              << "  montrer | show        afficher le plateau avec le dernier coup entre parentheses\n"
              << "  taille [n]            nouvelle partie sur un plateau n x n (5 a 19, komi conserve)\n"
              << "  captures | prisonniers afficher les pierres capturees par chaque joueur\n"
              << "  libertes <coord>      afficher les libertes du groupe contenant la pierre (ex: libertes D4)\n"
              << "  stats | statistiques  afficher les statistiques de la partie\n"
              << "  pierres [noir|blanc]  lister les coordonnees des pierres sur le plateau\n"
              << "  territoire            afficher le plateau avec les territoires (+ Noir, - Blanc)\n"
              << "  regles | rules        afficher les regles du jeu\n"
              << "  raccourcis            afficher les raccourcis clavier\n"
              << "  version | ver         afficher la version du jeu (aussi : ./go_game --version)\n"
              << "  credits               afficher les credits du jeu\n"
              << "  temps | time          afficher le temps de jeu (total et par joueur)\n"
              << "  joueurs               afficher le nom des joueurs\n"
              << "  nom noir|blanc <nom>  changer le nom d'un joueur\n"
              << "  date                  afficher la date et l'heure (et le debut de la partie)\n"
              << "  heure                 afficher l'heure actuelle\n"
              << "  fuseau                afficher le fuseau horaire\n"
              << "  langue                afficher la langue du jeu\n"
              << "  coups                 afficher le nombre de coups joues\n"
              << "  nbpierres             afficher le nombre de pierres sur le plateau\n"
              << "  aide | help | ?       afficher cette aide\n"
              << "  quit | q              quitter\n"
              << "Phase de marquage : coordonnee = marquer/demarquer un groupe mort, 'ok' = valider le score,\n"
              << "  'reprendre' = continuer la partie, 'annuler' = annuler la derniere passe,\n"
              << "  'score', 'nbpierres', 'coups', 'langue', 'fuseau', 'heure', 'date', 'joueurs', 'temps', 'credits', 'version', 'raccourcis', 'regles', 'territoire', 'pierres', 'stats', 'libertes', 'captures', 'montrer', 'komi', 'historique', 'dernier', 'sauvegarder' et 'aide' restent disponibles.\n\n";
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
    if (argc > 1 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        printVersion();
        return 0;
    }
    if (argc > 1) {
        std::string err;
        if (!game.loadFromFile(argv[1], player, err)) {
            std::cout << err << "\n";
            return 1;
        }
        gameClock.reset();
        size = game.getSize();
        std::cout << "Partie chargee depuis " << argv[1] << ".\n";
    } else {
        size = readBoardSize();
        if (size < 0) return 0;
        game = GoGame(size);
        gameClock.reset();
    }
    std::cout << "Tapez 'aide' pour voir les commandes.\n";

    bool finished = false;
    while (!finished) {
    while (!game.isOver()) {
        game.printBoard();
        std::cout << "Joueur " << player << " (" << nameOf(player) << ") > ";
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
        if (word == "komi") {
            komiCommand(game, arg);
            continue;
        }
        if (word == "montrer" || word == "show") {
            showLastMove(game);
            continue;
        }
        if (word == "captures" || word == "prisonniers") {
            printCaptures(game);
            continue;
        }
        if (word == "libertes" || word == "liberties") {
            libertiesCommand(game, arg);
            continue;
        }
        if (word == "stats" || word == "statistiques") {
            printStats(game, player);
            continue;
        }
        if (word == "pierres" || word == "stones") {
            printStones(game, arg);
            continue;
        }
        if (word == "territoire" || word == "territory") {
            game.printTerritory();
            continue;
        }
        if (word == "regles" || word == "rules") {
            printRules();
            continue;
        }
        if (word == "raccourcis" || word == "shortcuts") {
            printShortcuts();
            continue;
        }
        if (word == "version" || word == "ver") {
            printVersion();
            continue;
        }
        if (word == "credits") {
            printCredits();
            continue;
        }
        if (word == "temps" || word == "time") {
            printPlayTime(player, game.isOver());
            continue;
        }
        if (word == "joueurs" || word == "players") {
            printPlayers();
            continue;
        }
        if (word == "nom" || word == "name") {
            nameCommand(arg);
            continue;
        }
        if (word == "date") {
            printDate();
            continue;
        }
        if (word == "heure") {
            printTime();
            continue;
        }
        if (word == "fuseau" || word == "timezone") {
            printTimezone();
            continue;
        }
        if (word == "langue" || word == "language") {
            printLanguage();
            continue;
        }
        if (word == "coups" || word == "moves") {
            printMoveCount(game);
            continue;
        }
        if (word == "nbpierres") {
            printStoneCount(game);
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
        if (word == "taille" || word == "size") {
            sizeCommand(game, player, size, arg);
            continue;
        }
        if (word == "charger" || word == "load") {
            std::string err;
            std::string path = arg.empty() ? "partie.go" : arg;
            if (game.loadFromFile(path, player, err)) {
                gameClock.reset();
                size = game.getSize();
                std::cout << "Partie chargee depuis " << path << ".\n";
            } else {
                std::cout << err << "\n";
            }
            continue;
        }
        if (cmd == "undo" || cmd == "annuler" || cmd == "u") {
            if (game.undo(player)) { gameClock.pause(); std::cout << "Dernier coup annule.\n"; }
            else std::cout << "Rien a annuler.\n";
            continue;
        }
        if (cmd == "pass" || cmd == "p") {
            game.pass(player);
            gameClock.moveDone(player);
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
        gameClock.moveDone(player);
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
            if (word == "komi") {
                komiCommand(game, arg);
                continue;
            }
            if (word == "montrer" || word == "show") {
                showLastMove(game);
                continue;
            }
            if (word == "captures" || word == "prisonniers") {
                printCaptures(game);
                continue;
            }
            if (word == "libertes" || word == "liberties") {
                libertiesCommand(game, arg);
                continue;
            }
            if (word == "stats" || word == "statistiques") {
                printStats(game, player);
                continue;
            }
            if (word == "pierres" || word == "stones") {
                printStones(game, arg);
                continue;
            }
            if (word == "territoire" || word == "territory") {
                game.printTerritory();
                continue;
            }
            if (word == "regles" || word == "rules") {
                printRules();
                continue;
            }
            if (word == "raccourcis" || word == "shortcuts") {
                printShortcuts();
                continue;
            }
            if (word == "version" || word == "ver") {
                printVersion();
                continue;
            }
            if (word == "credits") {
                printCredits();
                continue;
            }
            if (word == "temps" || word == "time") {
                printPlayTime(player, game.isOver());
                continue;
            }
            if (word == "joueurs" || word == "players") {
                printPlayers();
                continue;
            }
            if (word == "nom" || word == "name") {
                nameCommand(arg);
                continue;
            }
            if (word == "date") {
                printDate();
                continue;
            }
            if (word == "heure") {
                printTime();
                continue;
            }
            if (word == "fuseau" || word == "timezone") {
                printTimezone();
                continue;
            }
            if (word == "langue" || word == "language") {
                printLanguage();
                continue;
            }
            if (word == "coups" || word == "moves") {
                printMoveCount(game);
                continue;
            }
            if (word == "nbpierres") {
                printStoneCount(game);
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
                gameClock.pause();
                resumed = true;
                std::cout << "Derniere passe annulee. Joueur " << player << " a la main.\n";
                break;
            }
            if (cmd == "reprendre" || cmd == "resume") {
                game.resume();
                gameClock.pause();
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
    std::cout << "Partie terminee. Score (aire + komi) - Noir : " << b << " | Blanc : " << w << "\n";
    std::cout << (b > w ? nameOf(GoGame::BLACK) : nameOf(GoGame::WHITE)) << " gagne" << " de " << (b > w ? b - w : w - b) << " points.\n";
    return 0;
}
