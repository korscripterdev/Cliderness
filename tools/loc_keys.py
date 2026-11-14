#!/usr/bin/env python3
"""Cliderness developer tool: list localisation keys used in src/ that are
missing from data/locale/en.lng (and keys in en.lng no longer used).

    python tools/loc_keys.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATTERN = re.compile(r'loc_(?:get|fmt|get_or)\(\s*"([a-z0-9_.]+)"')
LITERAL = re.compile(r'"((?:hud|msg|con|craft|chest|inv|eq|station|rcat|effect|cause|hint|set|lang|kb|help|menu|nw|lw|err|pause|stats|sky|mode|move|stat|skill|season|moon|ui|dim|itype|action|credits|mob|veh|ancient|fluid|npc|ai|diff|mk|agg)\.[a-z0-9_]+)"')


def used_keys():
    keys = set()
    for base, _dirs, files in os.walk(os.path.join(ROOT, "src")):
        for name in files:
            if name.endswith((".c", ".h")):
                text = open(os.path.join(base, name), encoding="utf-8").read()
                keys.update(PATTERN.findall(text))
                keys.update(LITERAL.findall(text))
    return keys


def lang_keys(code):
    keys = set()
    for line in open(os.path.join(ROOT, "data", "locale", code + ".lng"), encoding="utf-8"):
        line = line.strip()
        if line and not line.startswith(("#", "@")) and "=" in line:
            keys.add(line.split("=", 1)[0].strip())
    return keys


def main():
    used = used_keys()
    en = lang_keys("en")
    missing = sorted(k for k in used if k not in en)
    for k in missing:
        print(k)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
