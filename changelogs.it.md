# Registro delle modifiche

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · **Italiano**

Modifiche principali di Cliderness (creato da korscripterdev).

## [1.1.0] — 2026-11-15

Completa ciò che la 1.0.0 lasciava aperto nella tabella di marcia.

### Aggiunto
- Ricerca del percorso A* per le creature (heap binario e hash, sale 1 e scende 3 blocchi, conserva un percorso di 12 passi).
- Attributi che crescono giocando fino a 30: ogni abilità allena un attributo; Costituzione sale subendo danni (+2 PV max), Volontà sopportando gli estremi, Fortuna con bottino e colpi critici.
- Pesca: canna da pesca, abboccata dopo 3-8 secondi, probabilità 35% + Pesca / 2 + Fortuna / 4, reliquie nella Dimensione Antica.
- Abilità di furtività: passare accovacciato vicino a creature sveglie la allena e ti rende più silenzioso.
- Le navi vanno alla deriva con venti oltre il 25%, a meno che siano ancorate o tocchino la riva.
- Il Marinaio (coste) vende contratti di equipaggio; l'equipaggio rema senza resistenza (+10% ciascuno), svuota 2 l al secondo e pesa 75 kg. Cuccette: zattera 0, barca a remi 1, barca a vela 2, mercantile 4.
- Il cielo antico: la seconda luna Veyra (ciclo di 3 giorni), un mondo con anelli, il velo d'etere, stelle magenta e ciano.
- L'audio su Linux/BSD trasmette PCM mixato con 60 ms di anticipo.
- Guida in tutte le 11 lingue (63 argomenti) e nomi di blocchi, oggetti, biomi, meteo ed effetti.

### Modificato
- Le build DOS compilano e collegano nella CI (Open Watcom 16 bit e DOS/4GW); non ancora avviate su hardware o emulatore.
- Tabelle di lingua su DOS: 36000 byte e 960 voci.

## [1.0.0] — 2026-11-15

Fase 10 della roadmap (sistemi avanzati). Tutte e dieci le fasi sono implementate; ciò che resta da verificare è in ROADMAP.md.

### Aggiunto
- Audio PCM dal vivo: Windows waveOut (mixato con ciò che suona), Linux/BSD tramite PulseAudio, PipeWire o ALSA, macOS tramite afplay. Tutto è sintetizzato dal DSP procedurale.
- Passi, tuoni e suoni d'ambiente (pioggia, vento, tempesta, grotte, Dimensione Antica, uccelli, corvi, navi). `--play-sfx` prova il dispositivo.
- 119 nuovi comandi (240 in totale): forme di costruzione, informazioni, azioni uguali ai tasti, deposito, punti di riferimento, punto di morte, interfaccia e sistema.
- Pacchetti: regole install di CMake e CPack (ZIP, tar.gz), `tools/package.py` (con ZIP per DOS).

## [0.6.0] — 2026-11-14

Fase 9 della roadmap (la Dimensione Antica).

### Aggiunto
- Portali: crea un nucleo di portale all'incudine, piazzalo e premi E per viaggiare; viene costruito un portale di ritorno.
- La Dimensione Antica: seme proprio, 8 biomi (con fenditure del vuoto), blocchi e piante antichi, vene di etere e minerale di metallo stellare.
- 7 creature antiche, città e santuari antichi, l'altare antico con 9 ricette, reliquie.
- Meteo della dimensione (pioggia di stelle, nebbia silenziosa, vento d'etere), rinascita per dimensione. 5 nuovi comandi (121 in totale).

## [0.5.0] — 2026-11-14

Fase 8 della roadmap (navi).

### Aggiunto
- Zattera, barca a remi, barca a vela e nave mercantile (5x3 celle) con ricette.
- R per varare, E per salire/scendere, F per recuperare una nave vuota. Timone con i tasti di movimento (potenza -1..3, virata), ancora.
- I remi consumano resistenza; le vele usano direzione e forza del vento. Abilità di navigazione.
- Galleggiamento e portata, stive, sovraccarico. 3 stadi di danno dello scafo, falle, allagamento, sgottatura, riparazione, onde, incaglio, affondamento.
- Suoni di scricchiolio e schianto. 12 nuovi comandi (116 in totale).

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
