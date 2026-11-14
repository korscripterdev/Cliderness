#!/bin/sh
# Cliderness - MS-DOS / FreeDOS build with Open Watcom C (cross compile)
# Created by korscripterdev
#
#   builds/dos/build_watcom.sh        16-bit real mode, large model -> CLIDE16.EXE
#   builds/dos/build_watcom.sh 386    32-bit DOS/4GW                -> CLIDE386.EXE
#
# Requires wcc / wcc386 / wlink on PATH and WATCOM / INCLUDE set
# (e.g. by the open-watcom/setup-watcom GitHub action).
set -e
cd "$(dirname "$0")/../.."
# The DOS C library headers live in $WATCOM/h. setup-watcom on a Linux host
# points INCLUDE at $WATCOM/lh (the Linux headers), which cannot build DOS code.
if [ -n "$WATCOM" ]; then
  INCLUDE="$WATCOM/h"
  export INCLUDE
fi
T=${1:-16}
OBJ=builds/dos/obj$T
mkdir -p "$OBJ"
if [ "$T" = "386" ]; then
  CC=wcc386; MODEL=""; SYS=dos4g
else
  CC=wcc; MODEL="-ml -zt=2048"; SYS=dos
fi
LNK=$OBJ/link.lnk
{
  echo "system $SYS"
  echo "name builds/dos/CLIDE$T.EXE"
  echo "option quiet"
  echo "option stack=24k"
} > "$LNK"
LOG=$OBJ/build.log
: > "$LOG"
FAILED=0
for src in $(grep -v '^#' builds/dos/sources.lst); do
  o="$OBJ/$(basename "$src" .c).obj"
  case "$src" in
    src/platform/*) STRICT="" ;;   # OS headers: allow Watcom extensions
    *) STRICT="-za" ;;             # engine: strict ISO C (C89/C90)
  esac
  echo "  $CC $src"
  # compile every file even after an error, so one run reports all of them
  if ! $CC $MODEL -bt=dos $STRICT -ox -zq -w3 -i=src ${WATCOM:+-i="$WATCOM/h"} -fo="$o" "$src" > "$OBJ/cc.out" 2>&1; then
    FAILED=1
  fi
  cat "$OBJ/cc.out"
  cat "$OBJ/cc.out" >> "$LOG"
  echo "file $o" >> "$LNK"
done
annotate() {
  # GitHub Actions: one annotation per file with all of its messages
  [ -n "$GITHUB_ACTIONS" ] || return 0
  grep -E "$1" "$LOG" | sed -E 's/^([^(]+)\(([0-9]+)\): (.*)$/\1|\3 (line \2)/' |
    awk -F'|' -v lvl="$2" '{ m[$1] = m[$1] (m[$1] ? "%0A" : "") $2 }
      END { n = 0; for (f in m) if (n++ < 10) printf "::%s file=%s::%s\n", lvl, f, m[f] }'
}
if [ "$FAILED" -ne 0 ]; then
  annotate "Error!" error
  echo "DOS build failed: see the errors above" >&2
  exit 1
fi
annotate "Warning!" warning
if ! wlink @"$LNK" > "$OBJ/link.out" 2>&1; then
  cat "$OBJ/link.out"
  [ -n "$GITHUB_ACTIONS" ] && echo "::error::wlink: $(tr '\n' ' ' < "$OBJ/link.out" | cut -c1-3000)"
  exit 1
fi
cat "$OBJ/link.out"
ls -l "builds/dos/CLIDE$T.EXE"
