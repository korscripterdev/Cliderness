# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · **Italiano**

*Sandbox di sopravvivenza a mondo aperto* — creato da **korscripterdev**

Cliderness è un sandbox di sopravvivenza libero disegnato solo con caratteri CP437 in 16 colori VGA su una griglia 80×25. Un unico motore ISO C90 funziona su MS-DOS, FreeDOS, Windows, Linux, macOS e altri Unix. Niente trama né finale: esplora, scava, costruisci, rimodella il terreno e sopravvivi.

## Cosa funziona nella 1.1.0

* Mondo 3D deterministico da un seme (identico a 16 e 64 bit), 46 biomi, grotte, minerali, oceani, fiumi e laghi sotterranei; si salvano solo i chunk modificati.
* Camminare, correre, accovacciarsi, nuotare, scavare, piazzare, attrezzi con durabilità; salute, resistenza, fame, sete, fatica, temperatura corporea, ferite e malattie.
* Creazione (87 ricette), banco da lavoro, fornace, falò, incudine e altare, coltivazioni e alberi, forzieri, letti, equipaggiamento e zaini, riparazioni.
* ClideFluid: acqua e lava che scorrono conservando il volume, dighe, pozzanghere di pioggia, secchi; miniere abbandonate, rovine e città antiche.
* 30 creature (animali, mostri, PNG) con IA a obiettivi, vista e udito, ritmi di giorno e di notte; combattimento corpo a corpo e con l'arco, armature, bottino, commercio, 4 difficoltà.
* Navi: zattera, barca a remi, barca a vela e nave mercantile; remi e vele con il vento, stive, galleggiamento e sovraccarico, 3 stadi di danno dello scafo, allagamento, affondamento.
* La Dimensione Antica: portali, 8 biomi (con fenditure del vuoto), minerali, piante e creature antichi, l'altare antico.
* Stagioni e fasi lunari, 21 tipi di meteo, vista del cielo, suono procedurale (PCM dal vivo su Windows, Linux e macOS; PC Speaker su DOS).
* Novità della 1.1.0: ricerca del percorso A* per le creature, attributi che crescono giocando, pesca e furtività, deriva col vento ed equipaggi ingaggiati, la seconda luna e il mondo con gli anelli del cielo antico, audio mixato in streaming su Linux, guida e nomi in tutte le 11 lingue.
* **240** comandi, guida offline con 63 argomenti su 5 livelli, interfaccia in 11 lingue, salvataggio sicuro (file temporaneo + CRC).

Limiti: gli eseguibili DOS sono compilati e collegati dalla CI (Open Watcom, 16 bit e DOS/4GW) ma non ancora avviati su hardware reale o in un emulatore. L'audio dal vivo su Linux e macOS non è stato provato su dispositivi reali. Dettagli in [ROADMAP.md](ROADMAP.md).

## Compilare

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

Avvia `build/cliderness` (cerca `data/` accanto all'eseguibile o nella cartella corrente). `--help` elenca le opzioni, `--version` mostra versione e autore, `--play-sfx rain` prova il dispositivo audio.

## Comandi (predefiniti)

`WASD` muoversi · `Shift` correre · `Ctrl`/`X` accovacciarsi · `F` attaccare/scavare · `R` piazzare · `>`/`<` giù/su · `E` raccogliere/salire a bordo/commerciare · `U` usare · `K` creazione · `Tab` inventario · `C` personaggio · `Shift destro`/`V` cielo · `F1`/`?` guida · `Esc` pausa · `/` console. Tutti i tasti si possono cambiare.

## Documentazione

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Modifiche](changelogs.it.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Vedi [LICENSE](LICENSE).
