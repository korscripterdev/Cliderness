# Changelog

**English** · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

All notable changes to Cliderness (created by korscripterdev).

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
