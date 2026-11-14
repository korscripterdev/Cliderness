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

## What works in 1.0.0

* Seeded, deterministic 3D world (identical on 16-bit and 64-bit), 46 biomes,
  caves, ores, oceans, coral reefs, rivers and underground lakes; only changed
  chunks are saved.
* Walking, sprinting, crouching, swimming, climbing, falling; mining, placing,
  digging down, building up; tools with durability.
* Survival meters: HP, stamina, hunger, thirst, fatigue, body temperature;
  injuries, illness and buffs.
* 36-slot inventory + hotbar, dropped items that float or sink.
* Crafting (87 recipes), workbench, furnace, campfire, anvil and altar,
  farming and growing trees, chests, beds and sleep, equipment and bags, repairs.
* ClideFluid: flowing water and lava with volume conservation, dams, rain
  puddles, buckets; abandoned mines, ruins, ancient cities and shrines.
* 29 creatures (animals, monsters, NPCs) with goal-driven AI, sight and
  hearing, day/night routines; melee and bow combat, armour, loot, trading,
  undead that burn in daylight, four difficulty levels.
* Ships: raft, rowing boat, sailing boat, merchant ship; oars and sails with
  wind direction, cargo holds, buoyancy and overloading, three hull damage
  stages, leaks, flooding, bailing, repairs, waves, running aground, sinking.
* The Ancient Dimension: portals, eight biomes with void rifts, ancient ores,
  plants and creatures, its own sky and weather, the Ancient Altar with
  starmetal gear and relics.
* Game time with seasons and moon phases, 21 weather types, a full sky view
  (sun, moon, seeded constellations, clouds, birds, meteors, aurora, eclipses).
* Procedural sound: live PCM on Windows (waveOut, mixed), Linux/BSD
  (PulseAudio, PipeWire or ALSA) and macOS (afplay); PC Speaker tones on DOS.
* Command console: **240** working commands with permissions, Tab completion,
  history, undo/redo for building commands, waypoints and building shapes.
* Offline help: 62 topics, 5 levels, search, favourites, related links.
* 11 UI languages; F1 / Esc freeze the game completely.
* Crash-safe saves (temporary file + CRC verification + atomic replace).

Limits: the help and the names of blocks and items exist in English and Korean
only (the other UI languages show English there). The DOS executables are built
by CI but not yet run on real hardware or in an emulator. See
[ROADMAP.md](ROADMAP.md) for the exact status.

## Build

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test                     # Linux / macOS / Unix without CMake
sh builds/dos/build_watcom.sh 16      # MS-DOS (Open Watcom), see docs/platforms/dos.md
python tools/package.py               # release ZIP / tar.gz (CPack)
```

Run `build/cliderness` (it finds `data/` next to the executable or in the
current directory). `cliderness --help` lists the options; `--version` prints
the version and creator; `--play-sfx rain` checks the audio device.

## Controls (default)

`WASD` move · `Shift` sprint · `Ctrl`/`X` crouch · `F` attack / mine ·
`R` place · `>`/`<` down/up · `E` pick up / board / trade · `U` use ·
`K` crafting · `Tab` inventory · `C` character · `Right Shift`/`V` sky ·
`F1`/`?` help · `Esc` pause · `/` console. All keys can be rebound in Settings.

## Documentation

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) ·
[ROADMAP.md](ROADMAP.md) · [changelogs.md](changelogs.md) ·
[commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · See [LICENSE](LICENSE).
