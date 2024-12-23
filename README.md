## BattleShip

<img width="756" alt="Affichage Terminal Partie" src="https://github.com/user-attachments/assets/d02a4a5f-979b-40e6-b431-d4920b8d0c8e" />


## Auteurs

- Othman El Kazbani 493194
- FatimaZohra Lahrach 536142
- Jawad Cherkaoui 576517

## Compilation
Commentez les flags à la ligne 3 du Makefile si vous n'êtes pas sur Mac.

```make``` crée les programmes ```battleshipServer``` et ```battleshipClient```.

```make clean``` supprime les fichiers ```.o``` et ```.d```.

```make mrclean``` supprime les fichiers ```.o``` et ```.d``` et les exécutables.


## Comment fonctionne le programme ?

#### Serveur -> Client

Ces tokens sont envoyés au client pour des raisons d'affichage

| Action             | Tokens                                        |
| ------------------ | --------------------------------------------- |
| Add boat cell      | `20` `player` `coord_x`  `coord_y`    |
| Done placing boats | `21`                                        |
| Send fire          | `22` `coord_x`  `coord_y` `cell_type` |
| Receive fire       | `23` `coord_x`  `coord_y` `cell_type` |
| Update boards      | `24`                                        |
| Reset boards       | `28`                                            |
| Game over          | `29`                                        |

Ce que le serveur doit faire:

- le src/server/main.cc ne doit pas changer
- Comment les coups sont reçus ?
  Une fois un coup reçu, il est stocké dans le vecteur du joueur approprié (player_1/2_moves) et on indique que ce joueur vient de jouer un nouveau coup grâce à un booléen (client.has_new_move). Cela signifie que la partie attend constamment qu'un nouveau coup ait été donné avant de pouvoir le traiter (par le biais d'une boucle de type "while !client.has_new_move").
- Chaque bateau est composé de cellules ayant chacune une coordonnée. Pour chaque bateau,
  il faut envoyer un token selon le format de l'action `Add boat cell` pour chaque cellule de ce bateau. Le placement d'un bateau se termine en répétant le token `Add boat cell` avec la première coordonnée.
  Pour indiquer la fin du placement des bateaux d'un joueur, il faut lui envoyer un token selon le format `Done placing boats`. Il faut stocker ces coups dans un vecteur (players_moves) pour le mode observateur/replay en ajoutant comme premier token "1" ou "2" pour savoir à qui est le bateau (voir).
- Quand un joueur joue un coup valide, il faut lui envoyer un token `Send fire` avec les coordonnées du coup et le type de la cellule touchée. Il faut envoyer à l'autre joueur un token `Receive fire` avec les mêmes paramètres. Il faut aussi stocker ce coup dans un vecteur (players_moves) pour le mode observateur/replay.
- Les observateurs sont stockés sous forme d'un vecteur avec tous leurs sockets ("observers"). Lorsqu'un observateur se connecte on lui envoie tous les coups joués jusqu'à présent (pas les placements des bateaux). Ensuite, à chaque nouveau coup joué, on l'envoie à tous les observateurs. On l'envoie avec un token `Send/Receive fire` en fonction de si c'est le joueur 1 ou 2. Il faut aussi envoyer un token `Update boards` aux observateurs pour qu'ils affichent leurs boards actualisés.

| Command       | Tokens                          |
| ------------- |---------------------------------|
| Print message | `40` `split (0 or 1)` `message` |


| cell_type | Token |
| --------- | ----- |
| Water     | `0` |
| Ocean     | `1` |
| Undamaged | `2` |
| Hit       | `3` |
| Sunk      | `4` |

### Interface graphique

Quand on parle de "remplacer" pour les interfaces graphiques, cela signifie qu'il faut ajouter une nouvelle possibilité.
Ex:

```
if (TERMINAL_MODE){
    printf("blablabla");
} else{
    GUI.drawtext("blablabla");
}
```

#### Comment implémenter l'affichage sur interface graphique ?

- Remplacer tous les printf/cout dans src/client/main.cc par des affichages sur GUI
- Dans src/client/main.cc, dans la fonction receptionInfo, il faut remplacer l'appel à "(*param).update()" par un appel vers
  une nouvelle fonction qui affichera le plateau de jeu sur GUI

#### Comment lire les inputs sur interface graphique ?

- Remplacer tous les appels à "get_input" dans src/client/main.cc par une fonction qui lie des cliques et y associe des actions
