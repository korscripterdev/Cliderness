#!/usr/bin/env python3
"""Cliderness developer tool: check translated help files against en.hlp.

    python tools/help_check.py [lang ...]

Every language must have the same topics with the same level and related
links as English; command names (/word) used in English must appear too.
Exit code 1 on any difference.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def parse(code):
    path = os.path.join(ROOT, "data", "help", code + ".hlp")
    topics, cur = {}, None
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if line.startswith("@topic "):
            cur = line[7:].strip()
            topics[cur] = {"level": None, "related": "", "text": []}
        elif cur is None:
            continue
        elif line.startswith("@level "):
            topics[cur]["level"] = line[7:].strip()
        elif line.startswith("@related "):
            topics[cur]["related"] = line[9:].strip()
        elif not line.startswith("@"):
            topics[cur]["text"].append(line)
    return topics


def main():
    en = parse("en")
    langs = sys.argv[1:] or [f[:-4] for f in os.listdir(os.path.join(ROOT, "data", "help"))
                             if f.endswith(".hlp") and f not in ("en.hlp",)]
    bad = 0
    for code in sorted(langs):
        t = parse(code)
        problems = []
        for tid, e in en.items():
            if tid not in t:
                problems.append("missing topic " + tid)
                continue
            if t[tid]["level"] != e["level"]:
                problems.append("%s: level %s != %s" % (tid, t[tid]["level"], e["level"]))
            if t[tid]["related"] != e["related"]:
                problems.append("%s: related differs" % tid)
            cmds = set(re.findall(r"(?<![\w/])/[a-z]{3,}", "\n".join(e["text"])))
            body = "\n".join(t[tid]["text"])
            for c in sorted(cmds):
                if c not in body:
                    problems.append("%s: command %s not mentioned" % (tid, c))
            if not body.strip():
                problems.append("%s: empty" % tid)
        for tid in t:
            if tid not in en:
                problems.append("extra topic " + tid)
        print("%-6s %d topics, %s" % (code, len(t), "ok" if not problems else "%d problems" % len(problems)))
        for p in problems:
            print("   ", p)
        bad += len(problems)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
