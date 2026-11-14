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
grep -v '^#' builds/dos/sources.lst | while read -r src; do
  [ -z "$src" ] && continue
  o="$OBJ/$(basename "$src" .c).obj"
  case "$src" in
    src/platform/*) STRICT="" ;;   # OS headers: allow Watcom extensions
    *) STRICT="-za" ;;             # engine: strict ISO C (C89/C90)
  esac
  echo "  $CC $src"
  $CC $MODEL -bt=dos $STRICT -ox -zq -w3 -i=src -fo="$o" "$src"
  echo "file $o" >> "$LNK"
done
wlink @"$LNK"
ls -l "builds/dos/CLIDE$T.EXE"
