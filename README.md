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
- Score final par aire (pierres vivantes + territoires) avec komi reglable (6.5 par defaut)

## Compilation et execution

    g++ -std=c++17 -o go_game main.cpp GoGame.cpp
    ./go_game

## Jouer

- `D4` : colonne D, ligne 4 (ligne 1 en bas) ; ou `ligne colonne` (ex: `4 4`, depuis le haut gauche)
- `sauvegarder [fichier]` : sauvegarde la partie (defaut `partie.go`) ; `charger [fichier]` la recharge, avec annulation et ko conserves. On peut aussi lancer `./go_game partie.go`
- `score` : affiche le score en cours (aire + komi, estimation)
- `historique` (ou `histo`) : liste tous les coups joues (ex: `3. B D4`)
- `dernier` (ou `last`) : affiche le dernier coup joue
- `komi [valeur]` : affiche ou change le komi (defaut 6.5, sauvegarde avec la partie)
- `montrer` (ou `show`) : affiche le plateau avec le dernier coup entre parentheses, ex: `(B)`
- `taille [n]` : demarre une nouvelle partie sur un plateau n x n (5 a 19, komi conserve, confirmation si une partie est en cours)
- `captures` (ou `prisonniers`) : affiche les pierres capturees par chaque joueur
- `libertes <coord>` : affiche les libertes du groupe contenant la pierre (ex: `libertes D4`), signale l'atari
- `stats` (ou `statistiques`) : statistiques de la partie (coups, passes, pierres sur le plateau, captures, score estime)
- `pierres [noir|blanc]` : liste les coordonnees des pierres presentes sur le plateau
- `territoire` : affiche le plateau avec les territoires (`+` Noir, `-` Blanc) et leur taille
- `regles` (ou `rules`) : affiche les regles du jeu telles qu'implementees
- `raccourcis` : affiche les raccourcis (`p`, `u`, `q`, `h` et abreviations a deux lettres comme `sc` = score, `st` = stats)
- `version` (ou `ve`) : affiche la version du jeu ; `./go_game --version` fait de meme sans lancer la partie
- `credits` (ou `cr`) : affiche les credits et la licence (LGPL 2.1)
- `temps` (ou `tp`) : affiche le temps de jeu (duree totale et temps de reflexion de chaque joueur)
- `joueurs` (ou `jo`) : affiche le nom des joueurs ; `nom noir <nom>` / `nom blanc <nom>` les change (non sauvegarde dans le fichier de partie)
- `date` (ou `da`) : affiche la date et l'heure, et le moment ou la partie a commence
- `heure` (ou `he`) : affiche l'heure actuelle (HH:MM:SS)
- `fuseau` (ou `fu`) : affiche le fuseau horaire et le decalage UTC
- `aide` (ou `help`, `?`) : affiche la liste des commandes
- `pass` : passer ; `quit` : quitter
