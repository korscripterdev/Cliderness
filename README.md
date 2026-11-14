# Cliderness

**English** · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Open World Survival Sandbox* — created by **korscripterdev**.

Cliderness is a free-form survival sandbox drawn entirely with CP437 characters
in 16 VGA colours on an 80×25 grid. One ISO C90 engine targets MS‑DOS,
FreeDOS, Windows, Linux, macOS and other Unix systems. No story, no ending:
explore, mine, build, reshape the land and survive.

```text
~~~~~~~~~~~~~~≈≈≈≈.▒▒▓▓▓▓▓▓│HP      █████████  100
~~~~~"~~~~~≈≈≈≈≈..*▒▓▓·♦·▓▓│Stamina █████████ 100%
~~~~~~~~~≈≈≈≈@..¥.▒▓▓·····▓│Hunger  ████████·  90%
```

## What works in 0.4.0

* Seeded, deterministic 3D world (identical on 16-bit and 64-bit), 38 biomes,
  caves, ores, oceans, coral reefs; only changed chunks are saved.
* Walking, sprinting, crouching, swimming, climbing, falling; mining, placing,
  digging down, building up; tools with durability.
* Survival meters: HP, stamina, hunger, thirst, fatigue, body temperature.
* 36-slot inventory + hotbar, dropped items that float or sink.
* Crafting (58 recipes), workstations, smelting, cooking, farming and growing
  trees, chests, beds and sleep, equipment and bags, repairs, injuries and illness.
* ClideFluid: flowing water and lava with volume conservation, dams, rain
  puddles, buckets; rivers, underground lakes, abandoned mines and ruins.
* 22 creatures (animals, monsters, NPCs) with goal-driven AI, sight and
  hearing, day/night routines; melee and bow combat, armour, loot, trading,
  undead that burn in daylight, four difficulty levels.
* Game time with seasons and moon phases, 18 weather types, a full sky view
  (sun, moon, seeded constellations, clouds, birds, meteors, aurora, eclipses).
* Command console: **104** working commands with permissions, Tab completion,
  history and undo/redo for building commands.
* Offline help: 56 topics, 5 levels, search, favourites, related links.
* 11 UI languages; F1 / Esc freeze the game completely.
* Crash-safe saves (temporary file + CRC verification + atomic replace).

Not yet: ships, the Ancient
Dimension, live PCM audio. See [ROADMAP.md](ROADMAP.md) for the exact status.

## Build

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test                     # Linux / macOS / Unix without CMake
sh builds/dos/build_watcom.sh 16      # MS-DOS (Open Watcom), see docs/platforms/dos.md
```

Run `build/cliderness` (it finds `data/` next to the executable or in the
current directory). `cliderness --help` lists the options; `--version` prints
the version and creator.

## Controls (default)

`WASD` move · `Shift` sprint · `Ctrl`/`X` crouch · `F` mine · `R` place ·
`>`/`<` down/up · `E` pick up · `U` use · `Tab` inventory · `C` character ·
`Right Shift`/`V` sky · `F1`/`?` help · `Esc` pause · `/` console. All keys can
be rebound in Settings.

## Documentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) ·
[ROADMAP.md](ROADMAP.md) · [changelogs.md](changelogs.md) ·
[commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · See [LICENSE](LICENSE).
