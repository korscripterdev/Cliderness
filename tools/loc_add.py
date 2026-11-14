#!/usr/bin/env python3
"""Cliderness developer tool: merge translations into data/locale/<code>.lng.

    python tools/loc_add.py strings.json

strings.json = {"en": {"key": "value", ...}, "ko": {...}, ...}
Existing keys are replaced in place, new keys are appended under a
'# --- added ---' marker. Files stay UTF-8 with LF line endings.
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def merge(code, new):
    path = os.path.join(ROOT, "data", "locale", code + ".lng")
    lines = open(path, encoding="utf-8").read().split("\n")
    seen = set()
    for i, line in enumerate(lines):
        if "=" in line and not line.startswith(("#", "@")):
            k = line.split("=", 1)[0].strip()
            if k in new:
                lines[i] = "%s=%s" % (k, new[k])
                seen.add(k)
    while lines and lines[-1] == "":
        lines.pop()
    added = [k for k in new if k not in seen]
    if added:
        lines.append("")
        lines.append("# --- added ---")
        for k in added:
            lines.append("%s=%s" % (k, new[k]))
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    return len(added), len(seen)


def main():
    data = json.load(open(sys.argv[1], encoding="utf-8"))
    for code, strings in data.items():
        a, r = merge(code, strings)
        print("%-6s +%d new, %d replaced" % (code, a, r))


if __name__ == "__main__":
    main()
