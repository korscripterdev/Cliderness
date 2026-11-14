# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · **Deutsch** · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Open-World-Survival-Sandbox* — erstellt von **korscripterdev**

Cliderness ist eine freie Survival-Sandbox, die vollständig mit CP437-Zeichen in 16 VGA-Farben auf einem 80×25-Raster gezeichnet wird. Eine einzige ISO-C90-Engine läuft auf MS-DOS, FreeDOS, Windows, Linux, macOS und anderen Unix-Systemen. Keine Geschichte, kein Ende: erkunde, baue ab, baue auf, forme das Land um und überlebe.

## Was in 1.1.0 funktioniert

* Deterministische 3D-Welt aus einem Startwert (identisch auf 16 und 64 Bit), 46 Biome, Höhlen, Erze, Ozeane, Flüsse und unterirdische Seen; nur geänderte Chunks werden gespeichert.
* Gehen, Sprinten, Ducken, Schwimmen, Abbauen, Setzen, Werkzeuge mit Haltbarkeit; Gesundheit, Ausdauer, Hunger, Durst, Müdigkeit, Körpertemperatur, Verletzungen und Krankheiten.
* Handwerk (87 Rezepte), Werkbank, Ofen, Lagerfeuer, Amboss und Altar, Ackerbau und Bäume, Truhen, Betten, Ausrüstung und Taschen, Reparaturen.
* ClideFluid: fließendes Wasser und Lava mit Volumenerhaltung, Dämme, Regenpfützen, Eimer; verlassene Minen, Ruinen und uralte Städte.
* 30 Kreaturen (Tiere, Monster, NSC) mit zielgesteuerter KI, Sicht und Gehör, Tag- und Nachtrhythmus; Nah- und Bogenkampf, Rüstung, Beute, Handel, 4 Schwierigkeitsgrade.
* Schiffe: Floß, Ruderboot, Segelboot und Handelsschiff; Ruder und Segel mit Wind, Laderäume, Auftrieb und Überladung, 3 Rumpfschadensstufen, Wassereinbruch, Sinken.
* Die Uralte Dimension: Portale, 8 Biome (mit Leerenrissen), uralte Erze, Pflanzen und Kreaturen, der uralte Altar.
* Jahreszeiten und Mondphasen, 21 Wetterlagen, Himmelsansicht, prozeduraler Ton (Live-PCM unter Windows, Linux und macOS; PC-Lautsprecher unter DOS).
* Neu in 1.1.0: A*-Wegfindung für Kreaturen, Attribute, die beim Spielen wachsen, Angeln und Schleichen, Abdrift im Wind und angeheuerte Schiffsmannschaften, der zweite Mond und die Ringwelt am uralten Himmel, gemischtes Audio-Streaming unter Linux sowie Hilfe und Namen in allen 11 Sprachen.
* **240** Befehle, Offline-Hilfe mit 63 Themen in 5 Stufen, Oberfläche in 11 Sprachen, sicheres Speichern (Temporärdatei + CRC).

Grenzen: Die DOS-Programme werden in der CI gebaut und gelinkt (Open Watcom, 16 Bit und DOS/4GW), liefen aber noch nicht auf echter Hardware oder in einem Emulator. Live-Audio unter Linux und macOS ist auf echten Geräten nicht getestet. Details in [ROADMAP.md](ROADMAP.md).

## Bauen

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

Starte `build/cliderness` (sucht `data/` neben dem Programm oder im aktuellen Ordner). `--help` zeigt die Optionen, `--version` Version und Ersteller, `--play-sfx rain` prüft das Audiogerät.

## Steuerung (Standard)

`WASD` bewegen · `Shift` sprinten · `Strg`/`X` ducken · `F` angreifen/abbauen · `R` setzen · `>`/`<` runter/hoch · `E` aufheben/an Bord/handeln · `U` benutzen · `K` Handwerk · `Tab` Inventar · `C` Charakter · `rechte Umschalt`/`V` Himmel · `F1`/`?` Hilfe · `Esc` Pause · `/` Konsole. Alle Tasten sind frei belegbar.

## Dokumentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Änderungen](changelogs.de.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Siehe [LICENSE](LICENSE).
