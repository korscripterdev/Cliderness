# Cliderness on MS-DOS / FreeDOS

Created by korscripterdev.

## Status (0.1.0)

The DOS adapter (`src/platform/dos/dos_platform.c`) and build scripts exist and
the engine was written for 16-bit constraints, but **no DOS executable has
been built or run yet** — there is no Open Watcom or DJGPP toolchain on the
development machine. The GitHub Actions job `dos-watcom` builds both targets;
its first run is the first real compile. Treat DOS support as *untested*
until that job is green and the game has been run in DOSBox-X or FreeDOS.

## Targets

| target | compiler | executable | needs |
|---|---|---|---|
| 16-bit real mode, large model | Open Watcom `wcc -ml` | `CLIDE16.EXE` | 8086+ (386+ recommended), ~450 KB free conventional memory |
| 32-bit protected mode | Open Watcom `wcc386` | `CLIDE386.EXE` | 386+, DOS/4GW |
| 32-bit protected mode | DJGPP | `CLIDEDJ.EXE` | 386+, CWSDPMI |

These are genuine DOS executables, not Windows console programs.

## Building

```sh
sh builds/dos/build_watcom.sh 16      # or 386
make -f builds/dos/Makefile.dj        # DJGPP cross compiler
builds\dos\build.bat                  # Open Watcom on a Windows host
```

`builds/dos/sources.lst` lists every compiled file; the CTest
`dos_makefile_sync` fails when it is out of date.

## Running

Copy the executable and the `data` directory (all names are 8.3 safe, checked
by the CTest `dos_data_names`) into one directory and start it from there.
Saves go to `SAVES\<name>\`.

## How the adapter works

* **Video** — mode 03h (80×25), blink disabled for 16 background colours,
  cursor hidden, cells written straight to `B800:0000`; only changed cells.
* **Keyboard** — BIOS INT 16h enhanced services; shift-status polling turns a
  lone Right Shift or Ctrl press into the *look at sky* / *crouch* events.
* **Clock** — `clock()` (BIOS tick, ~55 ms); the fixed-step loop accumulates
  real time so the game speed stays correct.
* **Sound** — PC Speaker through PIT channel 2; tone sequences per event.
* **Files** — `rename()` cannot overwrite on DOS, so saves keep a `.bak` until
  the new file is in place.
* **Language** — CP437 only: English, Spanish, French, German and Italian are
  offered; Korean, Japanese, Chinese, Portuguese and Russian are hidden.

## Memory plan (16-bit)

| part | size |
|---|---|
| chunk cache (40 × 2 KB, separate allocations) | 82 KB |
| column cache (6 × 4 KB) | 24 KB |
| string tables (2 × 24 KB pool) | ~56 KB |
| help index | ~19 KB |
| framebuffers (2 × 8 KB) | 16 KB |
| chunk store index (≤ 2048 × 16 B) | ≤ 32 KB |
| edit journal (1024 records) | 16 KB |

`mem_set_budget(420000)` makes allocations fail cleanly instead of
corrupting memory; `/mem` reports usage in game.

## Known limits

* At most 2048 modified chunks per world on 16-bit DOS.
* Building natively on DOS needs a long-file-name driver (some source file
  names exceed 8.3); cross compiling is recommended.
* Performance on an 8086 will be poor; a 386 or better is the realistic target.
