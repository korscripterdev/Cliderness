# Änderungsprotokoll

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · **Deutsch** · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Wichtige Änderungen an Cliderness (erstellt von korscripterdev).

## [0.5.0] — 2026-11-14

Roadmap-Phase 8 (Schiffe).

### Hinzugefügt
- Floß, Ruderboot, Segelboot und Handelsschiff (5x3 Felder) mit Rezepten.
- R zum Ablegen, E zum An-/Von-Bord-Gehen, F holt ein leeres Schiff an Land. Steuern mit den Bewegungstasten (Leistung -1..3, Wenden), Anker.
- Ruder kosten Ausdauer; Segel nutzen Windrichtung und -stärke. Segel-Fertigkeit.
- Auftrieb und Tragfähigkeit, Laderäume, Überladung. 3 Rumpfschadensstufen, Lecks, Wassereinbruch, Schöpfen, Reparatur, Wellen, Auflaufen, Sinken.
- Knarr- und Bruchgeräusche. 12 neue Befehle (116 insgesamt).

## [0.4.0] — 2026-11-14

Roadmap-Phase 7 (Kreaturen und Kampf).

### Hinzugefügt
- 22 datengetriebene Kreaturen: 11 Tiere, 8 Monster und 3 NSC.
- Zielgesteuerte KI (überleben, angreifen, Schutz suchen, heimkehren, ruhen, umherstreifen) mit Sicht, Gehör, Wetter, Dunkelheit und Schleichen.
- Natürliches Erscheinen nach Biom, Tageszeit, Höhlen und Wasser; Verschwinden fern vom Spieler.
- Kampf: F greift an, Schwerter, Speer, Bogen und Pfeile, kritische Treffer, Rüstung, Blutung und Gift. Untote verbrennen im Tageslicht.
- 4 Schwierigkeitsgrade; Kreaturen, Abschüsse und Schwierigkeit werden gespeichert. Beute, gebratenes Fleisch, Lederrüstung, Knochenmehl, Feuerstein, 15 Rezepte.
- Handel mit NSC (E). 11 neue Befehle (104 insgesamt).

## [0.3.0] — 2026-11-14

Roadmap-Phase 6 (Weltsimulation).

### Hinzugefügt
- ClideFluid: fließendes Wasser und Lava mit Stufen 1-15, Volumenerhaltung, Meer und Seen füllen sich nach, Obsidian, Regenpfützen.
- Eimer, Flüsse im Tiefland und unterirdische Seen.
- Bauwerke: verlassene Minen und Steinruinen mit Beutekisten.
- 7 neue Befehle (93 insgesamt).

### Geändert
- Flüsse, Seen und Bauwerke verändern das unerkundete Gelände alter Welten.

## [0.2.0] — 2026-11-14

Roadmap-Phase 4 (Handwerk und Bauen) und der Rest von Phase 3.

### Hinzugefügt
- Handwerksbildschirm (K), 58 datengesteuerte Rezepte, Werkbank, Ofen, Lagerfeuer, Amboss und Brennstoff.
- Schmelzen, Kochen und Ackerbau (Hacken, Ackerland, Weizen und Kartoffeln, Setzlinge, die zu Bäumen wachsen).
- Truhen, Betten und Schlaf, Ausrüstungsplätze (Kopf, Körper, Beine, Füße, Rücken) und Reparatur.
- Zustände: Blutung, Knochenbruch, Lebensmittelvergiftung, Erkältung, nass, satt, ausgeruht und Heilmittel.
- 15 neue Befehle (86 insgesamt). Lizenz: GPL-3.0 oder später.

### Geändert
- Beschreibungen neuer Befehle erscheinen auf Englisch (die Oberfläche bleibt in 11 Sprachen übersetzt).

## [0.1.0] — 2026-11-14

Erste spielbare Grundlage (Phasen 1–3 fertig, 4–6 und 10 teilweise).

### Hinzugefügt
- ISO-C90-Engine mit Adaptern für die Windows-Konsole, POSIX-Terminals, MS-DOS/FreeDOS und einen Headless-Adapter für Tests.
- CP437- / VGA-16-Framebuffer mit Ausgabe geänderter Zellen und breiten Zeichen.
- Ganzzahlige Weltgenerierung aus einem Startwert, 38 Biome, Höhlen, Erze, Ozeane und sicherer Chunk-Speicher.
- Bewegung, Abbau, Platzieren, Werkzeuge und Haltbarkeit, Fertigkeiten, Überleben, Temperatur, Tod und Wiederbelebung.
- Inventar, Schnellleiste und Gegenstände mit Auftrieb.
- ClideTime, 18 Wetterlagen, Himmelsansicht; ClideConsole mit 71 Befehlen.
- Hilfe mit 45 Themen auf Englisch und Koreanisch, Oberfläche in 11 Sprachen, prozedurales DSP mit WAV-Export.
- Builds mit CMake, Make, Open Watcom und DJGPP; GitHub Actions; 293.610 Testprüfungen.

### Bekannte Einschränkungen
- Noch kein Handwerk, keine Kreaturen, kein Kampf, kein fließendes Wasser, keine Schiffe und keine Uralte Dimension.
- Kein Live-PCM-Ton unter Windows/Linux/macOS (läuft stumm).
- DOS-Versionen wurden noch nicht kompiliert oder gestartet (keine lokale Toolchain).
- Die Hilfe gibt es nur auf Englisch und Koreanisch; andere Sprachen zeigen die englische Hilfe.
