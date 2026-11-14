# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · **Français** · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Bac à sable de survie en monde ouvert* — créé par **korscripterdev**

Cliderness est un bac à sable de survie libre dessiné uniquement avec des caractères CP437 en 16 couleurs VGA sur une grille de 80×25. Un seul moteur ISO C90 fonctionne sous MS-DOS, FreeDOS, Windows, Linux, macOS et les autres Unix. Ni histoire ni fin : explorez, minez, construisez, remodelez le terrain et survivez.

## Ce qui fonctionne en 0.1.0

* Monde 3D déterministe à partir d'une graine (identique en 16 et 64 bits), 38 biomes, grottes, minerais, océans et récifs ; seuls les chunks modifiés sont enregistrés.
* Marche, course, accroupi, nage, escalade, chutes ; minage, pose, creuser vers le bas, construire vers le haut ; outils avec durabilité.
* Santé, endurance, faim, soif, fatigue, température corporelle ; inventaire de 36 cases avec barre rapide et objets qui flottent ou coulent.
* Temps de jeu avec saisons et phases de la lune, 18 météos, vue du ciel (soleil, lune, constellations, nuages, oiseaux, météores, aurores, éclipses).
* **71** commandes réellement fonctionnelles (permissions, complétion Tab, historique, annuler/rétablir).
* Aide hors ligne de 45 sujets sur 5 niveaux, interface en 11 langues ; F1 / Échap figent entièrement le jeu ; sauvegarde sûre (fichier temporaire + CRC).

Pas encore : artisanat, créatures et combat, eau qui coule, navires, la Dimension antique, audio PCM en direct. Statut exact dans [ROADMAP.md](ROADMAP.md).

## Compilation

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

Lancez `build/cliderness`. `--help` affiche les options, `--version` la version et l'auteur.

## Commandes (par défaut)

`WASD` se déplacer · `Shift` courir · `Ctrl`/`X` s'accroupir · `F` miner · `R` poser · `>`/`<` descendre/monter · `E` ramasser · `U` utiliser · `Tab` inventaire · `C` personnage · `Maj droite`/`V` ciel · `F1`/`?` aide · `Échap` pause · `/` console. Toutes les touches sont configurables.

## Documentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [journal](changelogs.fr.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Voir [LICENSE](LICENSE).
