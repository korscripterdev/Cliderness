# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · **Italiano**

*Sandbox di sopravvivenza a mondo aperto* — creato da **korscripterdev**

Cliderness è un sandbox di sopravvivenza libero disegnato solo con caratteri CP437 in 16 colori VGA su una griglia 80×25. Un unico motore ISO C90 funziona su MS-DOS, FreeDOS, Windows, Linux, macOS e altri Unix. Niente trama né finale: esplora, scava, costruisci, rimodella il terreno e sopravvivi.

## Cosa funziona nella 0.1.0

* Mondo 3D deterministico da seme (identico a 16 e 64 bit), 38 biomi, grotte, minerali, oceani e barriere coralline; si salvano solo i chunk modificati.
* Camminare, correre, accovacciarsi, nuotare, arrampicarsi e cadere; scavare, posare, scavare in basso e costruire in alto; attrezzi con durabilità.
* Salute, energia, fame, sete, fatica e temperatura corporea; inventario da 36 caselle con barra rapida e oggetti che galleggiano o affondano.
* Tempo di gioco con stagioni e fasi lunari, 18 tipi di meteo e vista del cielo (sole, luna, costellazioni, nuvole, uccelli, meteore, aurore, eclissi).
* **71** comandi realmente funzionanti (permessi, completamento con Tab, cronologia, annulla/ripeti).
* Aiuto offline con 45 argomenti su 5 livelli, interfaccia in 11 lingue; F1 / Esc fermano completamente il gioco; salvataggio sicuro (file temporaneo + CRC).

Non ancora: artigianato, creature e combattimento, acqua che scorre, navi, la Dimensione Antica e audio PCM dal vivo. Stato esatto in [ROADMAP.md](ROADMAP.md).

## Compilazione

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

Avvia `build/cliderness`. `--help` mostra le opzioni, `--version` versione e autore.

## Comandi (predefiniti)

`WASD` muoversi · `Shift` correre · `Ctrl`/`X` accovacciarsi · `F` scavare · `R` posare · `>`/`<` giù/su · `E` raccogliere · `U` usare · `Tab` inventario · `C` personaggio · `Maiusc destro`/`V` cielo · `F1`/`?` aiuto · `Esc` pausa · `/` console. Tutti i tasti sono riassegnabili.

## Documentazione

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [modifiche](changelogs.it.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Vedi [LICENSE](LICENSE).
