#!/usr/bin/env python3
"""Cliderness developer tool: replace or add help topics in data/help/<lang>.hlp.

    python tools/help_set.py topics.json

topics.json = {"en": [{"id":..., "title":..., "level":1, "related":"a,b", "text":"..."}], "ko": [...]}
A topic with an existing id is replaced in place; new topics are appended.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def render(t):
    out = ["@topic " + t["id"], "@title " + t["title"], "@level %d" % t["level"]]
    if t.get("related"):
        out.append("@related " + t["related"])
    out.append(t["text"].rstrip("\n"))
    return "\n".join(out) + "\n"


def main():
    data = json.load(open(sys.argv[1], encoding="utf-8"))
    for lang, topics in data.items():
        path = os.path.join(ROOT, "data", "help", lang + ".hlp")
        text = open(path, encoding="utf-8").read()
        parts = re.split(r"(?m)^(?=@topic )", text)
        index = {}
        for i, p in enumerate(parts):
            m = re.match(r"@topic (\S+)", p)
            if m:
                index[m.group(1)] = i
        for t in topics:
            block = render(t) + "\n"
            if t["id"] in index:
                parts[index[t["id"]]] = block
            else:
                if parts and not parts[-1].endswith("\n\n"):
                    parts[-1] = parts[-1].rstrip("\n") + "\n\n"
                parts.append(block)
                index[t["id"]] = len(parts) - 1
        out = "".join(parts).rstrip("\n") + "\n"
        open(path, "w", encoding="utf-8", newline="\n").write(out)
        print(lang, len(index), "topics")


if __name__ == "__main__":
    main()
