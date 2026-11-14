# Registro delle modifiche

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · **Italiano**

Modifiche principali di Cliderness (creato da korscripterdev).

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
