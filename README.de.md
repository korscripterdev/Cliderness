# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · **Deutsch** · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Open-World-Survival-Sandbox* — erstellt von **korscripterdev**

Cliderness ist eine freie Survival-Sandbox, die vollständig mit CP437-Zeichen in 16 VGA-Farben auf einem 80×25-Raster gezeichnet wird. Eine einzige ISO-C90-Engine läuft auf MS-DOS, FreeDOS, Windows, Linux, macOS und anderen Unix-Systemen. Keine Geschichte, kein Ende: erkunde, baue ab, baue auf, forme das Land um und überlebe.

## Was in 0.1.0 funktioniert

* Deterministische 3D-Welt aus einem Startwert (identisch auf 16 und 64 Bit), 38 Biome, Höhlen, Erze, Ozeane und Riffe; nur geänderte Chunks werden gespeichert.
* Gehen, Sprinten, Ducken, Schwimmen, Klettern, Fallen; Abbauen, Platzieren, nach unten graben, nach oben bauen; Werkzeuge mit Haltbarkeit.
* Gesundheit, Ausdauer, Hunger, Durst, Müdigkeit, Körpertemperatur; Inventar mit 36 Plätzen und Schnellleiste, Gegenstände schwimmen oder sinken.
* Spielzeit mit Jahreszeiten und Mondphasen, 18 Wetterlagen, Himmelsansicht (Sonne, Mond, Sternbilder, Wolken, Vögel, Meteore, Polarlichter, Finsternisse).
* **71** wirklich funktionierende Befehle (Rechte, Tab-Ergänzung, Verlauf, Rückgängig/Wiederholen).
* Offline-Hilfe mit 45 Themen in 5 Stufen, Oberfläche in 11 Sprachen; F1 / Esc halten das Spiel vollständig an; sicheres Speichern (Temporärdatei + CRC).

Noch nicht: Handwerk, Kreaturen und Kampf, fließendes Wasser, Schiffe, die Uralte Dimension, Live-PCM-Ton. Genauer Stand in [ROADMAP.md](ROADMAP.md).

## Bauen

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

Starte `build/cliderness`. `--help` zeigt die Optionen, `--version` Version und Ersteller.

## Steuerung (Standard)

`WASD` bewegen · `Shift` sprinten · `Strg`/`X` ducken · `F` abbauen · `R` setzen · `>`/`<` runter/hoch · `E` aufheben · `U` benutzen · `Tab` Inventar · `C` Charakter · `rechte Umschalt`/`V` Himmel · `F1`/`?` Hilfe · `Esc` Pause · `/` Konsole. Alle Tasten sind frei belegbar.

## Dokumentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Änderungen](changelogs.de.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Siehe [LICENSE](LICENSE).
