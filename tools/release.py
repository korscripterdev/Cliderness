#!/usr/bin/env python3
"""Cliderness developer tool for releases.

    python tools/release.py version 0.2.0        set the version everywhere
    python tools/release.py changelog entry.json add a changelog section

entry.json = {"en": "markdown of the new section", "ko": "...", ...}
The section is inserted above the newest '## [' heading of changelogs*.md.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CHANGELOG = {"en": "changelogs.md"}
for code in ["ko", "ja", "zh-CN", "zh-TW", "es", "fr", "de", "pt", "ru", "it"]:
    CHANGELOG[code] = "changelogs.%s.md" % code


def sub_file(rel, pattern, repl, count=1):
    path = os.path.join(ROOT, rel)
    text = open(path, encoding="utf-8").read()
    new, n = re.subn(pattern, repl, text, count=count)
    if n == 0:
        raise SystemExit("pattern not found in %s: %s" % (rel, pattern))
    open(path, "w", encoding="utf-8", newline="\n").write(new)


def set_version(v):
    major, minor, patch = (int(x) for x in v.split("."))
    num = major * 10000 + minor * 100 + patch
    sub_file("src/core/version.h", r'#define CLIDE_VERSION     "[^"]*"', '#define CLIDE_VERSION     "%s"' % v)
    sub_file("src/core/version.h", r"#define CLIDE_VERSION_NUM \d+L", "#define CLIDE_VERSION_NUM %dL" % num)
    sub_file("data/game.ini", r"version=.*", "version=" + v)
    sub_file("CMakeLists.txt", r"project\(Cliderness VERSION [0-9.]+", "project(Cliderness VERSION " + v)
    sub_file("data/help/en.hlp", r"Version [0-9.]+\. See the roadmap", "Version %s. See the roadmap" % v)
    sub_file("data/help/ko.hlp", r"버전 [0-9.]+\. 완료된", "버전 %s. 완료된" % v)
    print("version", v)


def add_changelog(path_json):
    data = json.load(open(path_json, encoding="utf-8"))
    for code, section in data.items():
        rel = CHANGELOG[code]
        path = os.path.join(ROOT, rel)
        text = open(path, encoding="utf-8").read()
        i = text.index("\n## [")
        text = text[:i + 1] + section.rstrip("\n") + "\n\n" + text[i + 1:]
        open(path, "w", encoding="utf-8", newline="\n").write(text)
        print("changelog", rel)


if __name__ == "__main__":
    if sys.argv[1] == "version":
        set_version(sys.argv[2])
    elif sys.argv[1] == "changelog":
        add_changelog(sys.argv[2])
