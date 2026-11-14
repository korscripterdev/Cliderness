# Registro delle modifiche

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · **Italiano**

Modifiche principali di Cliderness (creato da korscripterdev).

## [0.4.0] — 2026-11-14

Fase 7 della roadmap (creature e combattimento).

### Aggiunto
- 22 creature definite dai dati: 11 animali, 8 mostri e 3 PNG.
- IA a obiettivi (sopravvivere, attaccare, ripararsi, tornare a casa, riposare, vagare) con vista, udito, meteo, buio e furtività.
- Comparsa naturale per bioma, ora, grotte e acqua; scompaiono lontano dal giocatore.
- Combattimento: F attacca, spade, lancia, arco e frecce, colpi critici, armatura, sanguinamento e veleno. I non morti bruciano alla luce del giorno.
- 4 livelli di difficoltà; creature, uccisioni e difficoltà vengono salvate. Bottino, carne cotta, armatura di cuoio, farina d'ossa, selce, 15 ricette.
- Commercio con i PNG (E). 11 nuovi comandi (104 in totale).

## [0.3.0] — 2026-11-14

Fase 6 della roadmap (simulazione del mondo).

### Aggiunto
- ClideFluid: acqua e lava che scorrono con livelli 1-15, conservazione del volume, mari e laghi che si riempiono, ossidiana, pozzanghere di pioggia.
- Secchi, fiumi nelle pianure e laghi sotterranei.
- Strutture: miniere abbandonate e rovine di pietra con casse di bottino.
- 7 nuovi comandi (93 in totale).

### Modificato
- Fiumi, laghi e strutture cambiano il terreno inesplorato dei mondi vecchi.

## [0.2.0] — 2026-11-14

Fase 4 della roadmap (artigianato e costruzione) e il resto della fase 3.

### Aggiunto
- Schermata di artigianato (K), 58 ricette nei dati, banco da lavoro, fornace, falò, incudine e combustibile.
- Fusione, cucina e agricoltura (zappe, terreno arato, grano e patate, germogli che diventano alberi).
- Bauli, letti e sonno, slot di equipaggiamento (testa, corpo, gambe, piedi, schiena) e riparazione.
- Stati: emorragia, frattura, intossicazione, raffreddore, bagnato, sazio, riposato e rimedi.
- 15 nuovi comandi (86 in totale). Licenza: GPL-3.0 o successiva.

### Modificato
- Le descrizioni dei nuovi comandi appaiono in inglese (l'interfaccia resta tradotta in 11 lingue).

## [0.1.0] — 2026-11-14

Prima base giocabile (fasi 1–3 complete; 4–6 e 10 parziali).

### Aggiunto
- Motore ISO C90 con adattatori per la console di Windows, i terminali POSIX, MS-DOS/FreeDOS e un adattatore senza schermo per i test.
- Framebuffer CP437 / VGA-16 con output delle celle modificate e caratteri larghi.
- Generazione intera da seme, 38 biomi, grotte, minerali, oceani e archiviazione sicura dei chunk.
- Movimento, scavo, posa, attrezzi e durabilità, abilità, sopravvivenza, temperatura, morte e rinascita.
- Inventario, barra rapida e oggetti con galleggiamento.
- ClideTime, 18 tipi di meteo, vista del cielo; ClideConsole con 71 comandi.
- Aiuto di 45 argomenti in inglese e coreano, interfaccia in 11 lingue, DSP procedurale con esportazione WAV.
- Build con CMake, Make, Open Watcom e DJGPP; GitHub Actions; 293.610 controlli di test.

### Limiti noti
- Ancora niente artigianato, creature, combattimento, acqua che scorre, navi né Dimensione Antica.
- Niente audio PCM dal vivo su Windows/Linux/macOS (funziona in silenzio).
- Le versioni DOS non sono ancora state compilate né avviate (nessuna toolchain locale).
- L'aiuto esiste solo in inglese e coreano; le altre lingue usano quello inglese.
