# Journal des modifications

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · **Français** · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Changements notables de Cliderness (créé par korscripterdev).

## [0.5.0] — 2026-11-14

Phase 8 de la feuille de route (bateaux).

### Ajouté
- Radeau, barque, voilier et navire marchand (5x3 cases) avec recettes.
- R pour mettre à l'eau, E pour monter/descendre, F pour récupérer un bateau vide. Pilotage avec les touches de déplacement (puissance -1..3, virage), ancre.
- Les rames coûtent de l'endurance ; les voiles utilisent la direction et la force du vent. Compétence de navigation.
- Flottabilité et capacité, cales, surcharge. 3 stades de dégâts de coque, voies d'eau, écopage, réparation, vagues, échouage, naufrage.
- Sons de grincement et de craquement. 12 nouvelles commandes (116 au total).

## [0.4.0] — 2026-11-14

Phase 7 de la feuille de route (créatures et combat).

### Ajouté
- 22 créatures définies par les données : 11 animaux, 8 monstres et 3 PNJ.
- IA à objectifs (survivre, attaquer, s'abriter, rentrer, se reposer, errer) avec vue, ouïe, météo, obscurité et discrétion.
- Apparition naturelle selon le biome, l'heure, les grottes et l'eau ; disparition loin du joueur.
- Combat : F attaque, épées, lance, arc et flèches, coups critiques, armure, saignement et poison. Les morts-vivants brûlent au soleil.
- 4 niveaux de difficulté ; créatures, victimes et difficulté sauvegardées. Butin, viande cuite, armure de cuir, poudre d'os, silex, 15 recettes.
- Commerce avec les PNJ (E). 11 nouvelles commandes (104 au total).

## [0.3.0] — 2026-11-14

Phase 6 de la feuille de route (simulation du monde).

### Ajouté
- ClideFluid : eau et lave qui coulent avec des niveaux 1-15, conservation du volume, mer et lacs qui se remplissent, obsidienne, flaques de pluie.
- Seaux, rivières dans les basses terres et lacs souterrains.
- Structures : mines abandonnées et ruines de pierre avec caisses de butin.
- 7 nouvelles commandes (93 au total).

### Modifié
- Rivières, lacs et structures modifient le terrain inexploré des anciens mondes.

## [0.2.0] — 2026-11-14

Phase 4 de la feuille de route (artisanat et construction) et le reste de la phase 3.

### Ajouté
- Écran d'artisanat (K), 58 recettes en données, établi, four, feu de camp, enclume et combustible.
- Fonte, cuisine et agriculture (houes, terre labourée, blé et pommes de terre, pousses qui deviennent des arbres).
- Coffres, lits et sommeil, emplacements d'équipement (tête, corps, jambes, pieds, dos) et réparation.
- États : saignement, fracture, intoxication, rhume, mouillé, rassasié, reposé et leurs remèdes.
- 15 nouvelles commandes (86 au total). Licence : GPL-3.0 ou ultérieure.

### Modifié
- Les descriptions des nouvelles commandes s'affichent en anglais (l'interface reste traduite en 11 langues).

## [0.1.0] — 2026-11-14

Première base jouable (phases 1 à 3 terminées ; 4 à 6 et 10 partielles).

### Ajouté
- Moteur ISO C90 avec adaptateurs pour la console Windows, les terminaux POSIX, MS-DOS/FreeDOS et un adaptateur sans écran pour les tests.
- Framebuffer CP437 / VGA-16 avec sortie des cellules modifiées et caractères larges.
- Génération entière par graine, 38 biomes, grottes, minerais, océans et stockage sûr des chunks.
- Déplacement, minage, pose, outils et durabilité, compétences, survie, température, mort et réapparition.
- Inventaire, barre rapide et objets avec flottabilité.
- ClideTime, 18 météos, vue du ciel ; ClideConsole avec 71 commandes.
- Aide de 45 sujets en anglais et en coréen, interface en 11 langues, DSP procédural avec export WAV.
- Compilation CMake, Make, Open Watcom et DJGPP ; GitHub Actions ; 293 610 vérifications.

### Limitations connues
- Pas encore d'artisanat, de créatures, de combat, d'eau qui coule, de navires ni de Dimension antique.
- Pas d'audio PCM en direct sous Windows/Linux/macOS (fonctionne en silence).
- Les versions DOS n'ont pas encore été compilées ni lancées (pas d'outils locaux).
- L'aide n'existe qu'en anglais et en coréen ; les autres langues utilisent l'aide anglaise.
