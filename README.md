# go_game-
programme en C++ qui simule le jeu de Go connu en asie 

Le jeu de Go est un jeu de stratégie complexe qui se joue sur un plateau de 19x19 (ou parfois plus petit, comme 9x9 ou 13x13). Étant donné la complexité du jeu, un programme complet pour simuler le jeu de Go serait très long et complexe. Cependant, je vais vous montrer comment créer une version simplifiée du jeu en C++ qui vous permettra de placer des pierres sur un plateau et de vérifier les captures de base.

Concepts Clés
Plateau de Jeu : Un tableau 2D représentant le plateau de Go.
Placement des Pierres : Les joueurs placent des pierres sur le plateau.
Captures : Les pierres entourées sont capturées et retirées du plateau.

Explications
GoGame Class:

Constructor : Initialise le plateau de jeu avec des cases vides.
printBoard : Affiche le plateau de jeu.
placeStone : Place une pierre sur le plateau et vérifie les captures.
isCaptured : Vérifie si un groupe de pierres est capturé en utilisant une recherche par remplissage (flood fill).
removeCapturedStones : Supprime les pierres capturées du plateau.
floodFill : Utilisé pour vérifier les captures en recherchant les libertés autour des pierres.
Main Function:

Demande à l'utilisateur la taille du plateau.
Boucle de jeu où les joueurs placent des pierres tour à tour.
Affiche le plateau après chaque coup et change de joueur.
Compilation et Exécution
Assurez-vous que tous les fichiers sont dans le même répertoire, puis compilez et exécutez le programme avec :

g++ -o go_game main.cpp GoGame.cpp
./go_game

Ce programme est une version simplifiée du jeu de Go. Il peut être étendu pour inclure des fonctionnalités supplémentaires telles que la gestion des territoires, le calcul des scores, et une interface utilisateur graphique.
