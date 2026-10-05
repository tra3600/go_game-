# go_game-
programme en C++ qui simule le jeu de Go connu en asie 

Le jeu de Go est un jeu de stratégie complexe qui se joue sur un plateau de 19x19 (ou parfois plus petit, comme 9x9 ou 13x13). Étant donné la complexité du jeu, un programme complet pour simuler le jeu de Go serait très long et complexe. Cependant, je vais vous montrer comment créer une version simplifiée du jeu en C++ qui vous permettra de placer des pierres sur un plateau et de vérifier les captures de base.

## Fonctionnalites

- Plateau de 5x5 a 19x19 avec coordonnees (colonnes A-T sans I, lignes numerotees)
- Captures de groupes (par flood fill), compteur de captures
- Interdiction du suicide et regle du ko (superko positionnel)
- Passer son tour ; deux passes consecutives terminent la partie
- `annuler` : annule le dernier coup ou la derniere passe (autant de fois que voulu, y compris depuis la phase de marquage)
- Marquage des pierres mortes apres les deux passes (groupe entier, bascule par clic de coordonnee, `ok` pour valider, `reprendre` pour continuer la partie en cas de desaccord)
- Score final par aire (pierres vivantes + territoires) avec komi 6.5

## Compilation et execution

    g++ -std=c++17 -o go_game main.cpp GoGame.cpp
    ./go_game

## Jouer

- `D4` : colonne D, ligne 4 (ligne 1 en bas) ; ou `ligne colonne` (ex: `4 4`, depuis le haut gauche)
- `pass` : passer ; `quit` : quitter
