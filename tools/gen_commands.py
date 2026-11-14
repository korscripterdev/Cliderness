#!/usr/bin/env python3
"""Cliderness developer tool: regenerate docs/commands/commands.md.

    python tools/gen_commands.py path/to/cliderness[.exe]

Runs `cliderness --list-commands` and writes the markdown table.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

HEADER = """# Cliderness command reference

Generated with `cliderness --list-commands`. Created by korscripterdev.

Every command below has a real handler and is covered by the test suite. Cheat commands need `/cheats on` (or *Allow cheats* when creating the world). `/help <command>` shows the localised description in game.

| command | category | permission | syntax |
|---|---|---|---|
"""


def main():
    exe = os.path.abspath(sys.argv[1])
    out = subprocess.run([exe, "--data", os.path.join(ROOT, "data"), "--list-commands"],
                         capture_output=True, text=True, encoding="utf-8").stdout
    rows, total = [], ""
    for line in out.splitlines():
        if line.startswith("total"):
            total = line.strip()
            continue
        m = re.match(r"^(/\S+)\s+(\S+)\s+(\S+)\s?(.*)$", line)
        if not m:
            continue
        name, cat, perm, syntax = m.groups()
        syntax = syntax.strip().replace("|", "\\|")
        rows.append("| `%s` | %s | %s | %s |" % (name, cat, perm, ("`%s`" % syntax) if syntax else ""))
    text = HEADER + "\n".join(rows) + "\n\n" + total + "\n"
    path = os.path.join(ROOT, "docs", "commands", "commands.md")
    open(path, "w", encoding="utf-8", newline="\n").write(text)
    print("%d commands -> %s" % (len(rows), path))


if __name__ == "__main__":
    main()
