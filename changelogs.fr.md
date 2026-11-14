# Journal des modifications

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · **Français** · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Changements notables de Cliderness (créé par korscripterdev).

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
