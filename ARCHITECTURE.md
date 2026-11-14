# Cliderness — Architecture

Engine: **Cliderness Engine**, ISO C90. Created by korscripterdev.

## Layers

```text
 src/main.c
   └─ ui/            app shell, screen stack, fixed-step loop, screens
        ├─ commands/ ClideConsole: registry, parser, completion, handlers
        ├─ help/     ClideHelp: streamed help index
        ├─ gameplay/ Game: player, inventory, survival, crafting, farming, chests, effects,
        │            creatures + AI (entities/), ships (vehicles/), portals (dimension/), undo journal
        │    └─ world/   World: chunk cache, terrain, biomes, structures, ClideFluid, time, weather, sky, chunk store
        ├─ save/     world.ini (CRC) + chunk store commit
        ├─ content/  blocks, items, recipes, effects from data/
        ├─ localization/ ClideLocale: string tables, display checks
        ├─ audio/    ClideAudio front end (PCM synthesis + mixing, or tones) + fixed-point DSP
        ├─ renderer/ ClideRenderer: framebuffer, CP437 table, text/wide chars
        ├─ input/    key events, rebindable actions
        └─ core/     types, integer math, rng, noise, strings, ini, crc, log, memory
 platform/  ClidePlatform implementations (the only OS-specific code)
   windows/ Win32 console + waveOut   unix/ POSIX termios + ANSI + PCM pipe   dos/ VGA text + BIOS + PC speaker
   null/    headless (tests, --headless)
```

Game logic never calls an OS API. `platform/platform.h` defines
`ClideTerminal`, `ClideInput`, `ClideClock`, `ClideFilesystem`, `ClideAudio`,
`ClideLocale`, `ClideMemory`; one `ClidePlatform` is chosen at start-up.

## C90 rules

* No `//`, no mixed declarations, no `long long`, no `stdint.h`, no `snprintf`.
* `int` may be 16 bits: anything above 32767 is `long`; code points are `long`.
* `unsigned long` may be 64 bits: hashes are masked with `CL_U32()`.
* C90 leaves negative division / right shift implementation-defined:
  `mx_floor_div` / `mx_floor_mod` are used instead.
* No floating point in the simulation (FPU-less DOS, bit-identical worlds).
* No single allocation above 64 KB (16-bit `size_t`).
* Enforced by `-std=c90 -pedantic-errors` (clang, gcc) and Watcom `-za`.
  Platform adapters include OS headers and are compiled as C99 but written in
  C90 style.

## World storage

* Axes: x east, z south, y up (0..127), sea level 62.
* **Chunk** = 32×32 cells of one y level of one dimension:
  1024 block ids + 1024 meta bytes (fluid level, `META_PLACED`).
* LRU chunk cache (768 chunks desktop / 40 DOS), hash buckets, last-hit cache.
* Column cache holds per-column height / biome / climate (48 / 6).
* `terrain_block()` is a pure function → seamless chunk borders.
* Chunk store: only `CH_MODIFIED` chunks. Evicted chunks go to a session
  journal `chunks.jnl`; a save rewrites `chunks.new`, re-reads and verifies
  every record (CRC-32), then atomically replaces `chunks.dat`.

## Save files (`saves/<name>/`, DOS 8.3 safe)

| file | content |
|---|---|
| `world.ini` | text, sections world/time/weather/player/inventory/equipment/effects/crops/chests/fluid/waypoints/ships/mobs/entities, trailing `crc=` line |
| `chunks.dat` | `CLCHUNK1` header + RLE records with per-record CRC |
| `../worlds.lst` | world list (no directory listing needed in C90) |

Write → temp file → verify → replace (MoveFileEx / rename / DOS .bak dance).

## Main loop

```text
loop:
  dt = clock delta (capped 250 ms)
  poll input → top screen
  if game active and no pausing screen: accum += dt; run ticks of 50 ms (max 10)
  else: accum = 0
  draw screens from the top-most opaque one → back buffer
  present (only dirty cells) → sleep to ~30 fps (DOS ~18 fps)
```

## Data files

| path | parser | notes |
|---|---|---|
| `data/blocks/*.def` | content.c | glyph, colours, flags, hardness, tool, drop, ore generation |
| `data/items/*.def` | content.c | blocks automatically get block items |
| `data/biomes/*.def` | biome.c | first match wins (height, temp, humidity, special) |
| `data/weather/weather.def` | weather.c | climate ranges, effects, durations, dimension |
| `data/recipes/*.def` | recipes.c | inputs, station, fuel, skill |
| `data/effects/effects.def` | effects.c | status effects |
| `data/structs/structs.def` | structure.c | mines, ruins, cities, loot tables |
| `data/entities/creature.def` | creature.c | creatures, AI parameters, trades, loot tables |
| `data/vehicles/ships.def` | ship.c | hull, oars, sails, capacity, wave limit |
| `data/locale/*.lng` | locale.c | `key=value`, `@name/@native/@script/@sample` |
| `data/help/*.hlp` | help.c | `@topic/@title/@level/@related`, streamed |
| `data/config/time.ini` | app.c | ClideTime defaults |

Every directory has an `index.lst` because C90 cannot list directories.

## Simulation per tick

```text
player (move, mine, gravity, survival) → item entities → creatures (think every
10 ticks, act by speed) → natural spawning (every 40 ticks) → ships (speed,
movement; sea, leaks and sinking once per second) → ClideFluid (every 5 ticks)
→ per game minute: survival, crops, rain, weather
```

Audio requests from the simulation go through `Game.sfx`; the UI plays them
and schedules ambience. On back ends that can restart output (waveOut,
headless) a new sound is mixed into the rest of the current one.

## Memory budget (16-bit DOS)

Chunk cache 40 × 2 KB, columns 6 × 4 KB, locale pools 2 × 32 KB (+ 840-entry index each), help index
≈19 KB, store index ≤ 32 KB, edit journal 16 KB. `mem_set_budget(420000)` is
enforced on 16-bit builds; `/mem` shows live usage.
