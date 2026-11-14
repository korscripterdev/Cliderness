# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · **Français** · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Bac à sable de survie en monde ouvert* — créé par **korscripterdev**

Cliderness est un bac à sable de survie libre dessiné uniquement avec des caractères CP437 en 16 couleurs VGA sur une grille de 80×25. Un seul moteur ISO C90 fonctionne sous MS-DOS, FreeDOS, Windows, Linux, macOS et les autres Unix. Ni histoire ni fin : explorez, minez, construisez, remodelez le terrain et survivez.

## Ce qui fonctionne dans la 1.1.0

* Monde 3D déterministe à partir d'une graine (identique en 16 et 64 bits), 46 biomes, grottes, minerais, océans, rivières et lacs souterrains ; seuls les chunks modifiés sont sauvegardés.
* Marcher, courir, s'accroupir, nager, miner, poser, outils avec durabilité ; santé, endurance, faim, soif, fatigue, température corporelle, blessures et maladies.
* Artisanat (87 recettes), établi, four, feu de camp, enclume et autel, cultures et arbres, coffres, lits, équipement et sacs, réparations.
* ClideFluid : eau et lave qui coulent en conservant le volume, barrages, flaques de pluie, seaux ; mines abandonnées, ruines et cités anciennes.
* 30 créatures (animaux, monstres, PNJ) avec une IA à objectifs, vue et ouïe, rythmes jour/nuit ; combat au corps à corps et à l'arc, armures, butin, commerce, 4 difficultés.
* Bateaux : radeau, barque, voilier et navire marchand ; rames et voiles selon le vent, cales, flottabilité et surcharge, 3 stades de dégâts de coque, voies d'eau, naufrage.
* La Dimension Ancienne : portails, 8 biomes (dont les failles du vide), minerais, plantes et créatures anciens, l'autel ancien.
* Saisons et phases de lune, 21 types de météo, vue du ciel, son procédural (PCM en direct sous Windows, Linux et macOS ; haut-parleur PC sous DOS).
* Nouveau en 1.1.0 : recherche de chemin A* pour les créatures, attributs qui progressent en jouant, pêche et discrétion, dérive au vent et équipages engagés, la seconde lune et le monde à anneaux du ciel antique, audio mixé en flux sous Linux, aide et noms dans les 11 langues.
* **240** commandes, aide hors ligne de 63 sujets sur 5 niveaux, interface en 11 langues, sauvegarde sûre (fichier temporaire + CRC).

Limites : les exécutables DOS sont compilés et liés par la CI (Open Watcom, 16 bits et DOS/4GW) mais pas encore lancés sur une vraie machine ni dans un émulateur. L'audio en direct sous Linux et macOS n'a pas été essayé sur de vrais appareils. Détails dans [ROADMAP.md](ROADMAP.md).

## Compiler

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

Lancez `build/cliderness` (il cherche `data/` à côté de l'exécutable ou dans le dossier courant). `--help` liste les options, `--version` affiche la version et l'auteur, `--play-sfx rain` teste le périphérique audio.

## Commandes (par défaut)

`WASD` se déplacer · `Maj` courir · `Ctrl`/`X` s'accroupir · `F` attaquer/miner · `R` poser · `>`/`<` descendre/monter · `E` ramasser/embarquer/commercer · `U` utiliser · `K` artisanat · `Tab` inventaire · `C` personnage · `Maj droite`/`V` ciel · `F1`/`?` aide · `Échap` pause · `/` console. Toutes les touches sont modifiables.

## Documentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Changements](changelogs.fr.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Voir [LICENSE](LICENSE).
