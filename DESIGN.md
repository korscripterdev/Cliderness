# Cliderness — Design

**Cliderness** is an open world survival sandbox created by **korscripterdev**.
It runs in an 80×25 character grid (CP437 glyphs, 16 VGA colours) on MS‑DOS,
FreeDOS, Windows, Linux, macOS and other Unix systems, from a single ISO C90
code base.

## Pillars

1. **Free-form sandbox** — no story, no mandatory goals, no ending.
2. **Everything is changeable** — every block of the 3D world can be mined,
   placed, flattened, flooded or drained. Only changes are saved; the rest is
   regenerated from the seed.
3. **Information-dense interface** — Cataclysm: DDA–style side panel with
   every survival number visible, a hotbar and an event log.
4. **Same game everywhere** — one logical CP437 / VGA‑16 screen and identical
   integer simulation on 16‑bit DOS and 64‑bit desktops. No platform gets a
   "better" graphics mode.
5. **Data-driven content** — blocks, items, biomes, weather, strings and help
   are text files in `data/`. Adding a biome or a block needs no code.
6. **Honest scope** — systems that are not implemented are labelled
   "planned" in the game's help and in `ROADMAP.md`.

## Screen layout (80×25)

```text
cols 0-55 / rows 0-18   world view (top-down, player centred)   | cols 57-79 status panel (P: 3 pages)
row 19                  hotbar 1-9 + selected item name
rows 20-24              event log (newest at the bottom, repeats merged "x3")
```

Overlays: pause menu (Esc), help (F1/?), console (/), inventory (Tab),
character sheet (C), sky view (Right Shift / V).

## Reading the map

The player stands on one height level. Cells show, in order of priority:

| situation | drawn as |
|---|---|
| block at eye level, solid, with air above (a step) | its top face, bright |
| block at eye level, 2 high / 3+ high (cliff) | `▒` / `▓` |
| trees, plants, built blocks at eye level | their glyph (`♠ ♣ ¥ * #`) |
| air at eye level | the first block below, darker with depth |
| out of light | black |

## Time and pausing

* 20 simulation ticks per second, fixed step.
* Default: 1 game day = 1200 real seconds (`data/config/time.ini`).
* Help and the pause menu set `Screen.pauses`; while any such screen is on the
  stack no real time is accumulated, so nothing — time, weather, survival,
  physics — advances, and nothing is "replayed" on resume. Verified by
  `tests/test_app.c`.

## Survival model (0.1.0)

Meters use 0..1000 integers. Per game minute: hunger −0.5 (−1 if you
sprinted), thirst −0.7 (−1 above 28 °C), fatigue +0.4. Body temperature drifts
toward the environment outside 10–28 °C, damped by Willpower. Formulas are
documented in the in-game help topic **Formulas (Master)**.

## Anti-exploit rules

* Blocks placed by a player carry `META_PLACED`; mining them gives no XP.
* Exploration XP only for new maximum distance from spawn.
* At most 60 skill XP per game minute.

## Game modes vs. cheats

Modes (survival, creative, adventure, spectator) define rules. Cheats are a
separate per-world flag; any cheat use sets the saved `cheated` flag.
God / fly / noclip are independent flags.

## Localisation

Base language English; 11 languages ship with complete UI tables.
A CP437 display (DOS) only offers languages whose every string fits CP437
(en, es, fr, de, it). On Unicode terminals, CJK and Hangul are marked
*unverified* and the player confirms a sample sentence before switching.
