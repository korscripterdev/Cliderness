# Changelog

**English** · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

All notable changes to Cliderness (created by korscripterdev).

## [0.4.0] — 2026-11-14

Roadmap phase 7 (creatures and combat).

### Added
- 22 data-driven creatures (`data/entities/creature.def`): deer, boars, rabbits, sheep, chickens, wolves, bears, camels, fish, sharks, bats; zombies, skeletons, giant spiders, cave crawlers, slimes, bog wraiths, sand scorpions, frost ghouls; wandering traders, hermits and old miners.
- Goal-driven AI state machine (survive, attack, shelter, go home, rest, wander) with sight, hearing, weather visibility, darkness and crouching; day and night routines, NPCs return home at dusk.
- Natural spawning by biome, time of day, caves and water; creatures despawn far from the player.
- Combat: F attacks the creature in front of you, swords, flint spear, bow and arrows, critical hits, weapon cooldowns and wear, Melee/Ranged skill xp. Creatures hit back, armour reduces damage and wears, some creatures cause bleeding or poison. Undead burn in daylight.
- Difficulty (peaceful, easy, normal, hard), saved with the world. Creatures, kills and difficulty are saved.
- Loot tables for creatures; raw/cooked meat and fish, leather armour, bone meal (fertiliser), flint from gravel; 15 new recipes.
- NPC trading with E.
- 11 commands (104 in total): summon, creatures, mobs, difficulty, spawning, despawn, bestiary, attack, trade, threat, kills; /kill @e[type=<creature>] now works.
- Help: combat, creatures, creature behaviour, difficulty, trading; formulas updated.

### Changed
- You cannot sleep with a hostile creature within 12 cells (this check now has real creatures).

## [0.3.0] — 2026-11-14

Roadmap phase 6 (world simulation).

### Added
- ClideFluid: flowing water and lava with levels 1-15, volume conservation, edge flow into holes, levelling of pools, active-cell queue with a per-step budget (small on DOS). Sea and lake sources refill; water and lava form obsidian or cobblestone; rain fills dents in the ground.
- Buckets (scoop and pour a cell; the amount is kept in the bucket).
- Rivers cut below sea level through low land; underground lakes in deep caves.
- Structures (`data/structs`): abandoned mines with corridors, beams, torches and loot crates; stone ruins with a crate. Loot tables fill crates when first opened.
- 7 commands (93 in total): fluid, water, lava, settle, volume, structures, findstructure.
- Help: fluids, buckets, structures, rivers. Active fluid cells are saved.

### Changed
- World generation now includes rivers, lakes and structures: unexplored terrain of older worlds differs from 0.2.0.

## [0.2.0] — 2026-11-14

Roadmap phase 4 (crafting and building) and the phase 3 leftovers.

### Added
- Crafting screen (K) with 58 data-driven recipes (`data/recipes`), categories, search, craft-all.
- Stations: workbench, furnace, campfire, anvil; fuel units (coal/charcoal 8, logs 4, planks 2, sticks 1).
- Smelting (iron, copper, tin, silver, gold, bronze ingots, glass, bricks, charcoal) and cooking (bread, baked potato, mushroom stew, roasted berries, herbal tea).
- Farming: hoes, farmland, wheat and potato crops with growth stages (faster in rain, slower in winter); saplings that grow into trees.
- Chests (27 slots), beds and sleeping until morning (sets the spawn point).
- Equipment slots (head, body, legs, feet, back): fiber clothes, iron armour, fiber bag; defense, warmth and extra carry capacity.
- Repairing tools and clothes with materials at a workbench or anvil.
- Status effects (`data/effects`): bleeding, fracture, food poisoning, common cold, wet, well fed, rested; bandage, splint, charcoal pill, herbal tea.
- 15 commands (86 in total): craft, recipes, recipe, stations, fuel, effect, cure, sleep, skipnight, grow, crops, equip, unequip, equipment, chests.
- Help topics: crafting, stations, farming, chests, equipment, sleep, effects, durability. Save format 2 (format 1 saves still load).
- License: GPL-3.0-or-later.

### Changed
- Command descriptions added after 0.1.0 are translated into English and Korean; other languages show them in English. All UI text remains translated in 11 languages.

## [0.1.0] — 2026-11-14

First playable foundation (roadmap phases 1–3 complete, 4–6 and 10 partial).

### Added
- ISO C90 engine with platform adapters for Windows console, POSIX terminals
  (Linux, macOS, BSD), MS-DOS/FreeDOS (VGA text, BIOS keyboard, PC Speaker)
  and a headless adapter for tests.
- CP437 / VGA-16 framebuffer with dirty-cell output and wide-character text.
- Seeded integer world generation: 3D chunks, 38 data-driven biomes, caves,
  ores, oceans; chunk cache and a crash-safe chunk store.
- Player movement, sprint, crouch, swimming, climbing, fall damage; mining,
  placing, tools and durability; skills with anti-farming rules.
- Survival meters, body temperature, death and respawn.
- Inventory, hotbar, dropped items with buoyancy.
- ClideTime (seasons, moon phases, time scale), 18 weather types, sky view.
- ClideConsole with 71 commands, permissions, completion, history, undo/redo.
- ClideHelp with 45 topics in English and Korean; UI in 11 languages.
- Procedural DSP sound library with WAV export (`--render-sfx`).
- CMake, Make, Open Watcom and DJGPP builds; GitHub Actions; 293,610 test checks.

### Known limitations
- No crafting, creatures, combat, flowing water, ships or Ancient Dimension yet.
- No live PCM audio on Windows/Linux/macOS (runs silently).
- DOS builds have not been compiled or run yet (no local toolchain).
- Help is available in English and Korean only; other languages fall back to English help.
