#!/usr/bin/env python3
"""Cliderness developer tool: build release packages.

    python tools/package.py [build-dir]          native ZIP / .tar.gz via CPack
    python tools/package.py --dos                 DOS folder from builds/dos/*.EXE

The native package contains the executable, data/, docs/, LICENSE, all
README and changelog languages. The DOS package uses 8.3 names (CLIDE/DATA)
and needs CLIDE16.EXE / CLIDE386.EXE built by builds/dos/build_watcom.sh.
"""
import os
import shutil
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run(cmd, cwd=None):
    print("+", " ".join(cmd))
    subprocess.check_call(cmd, cwd=cwd)


def native(build):
    build = os.path.abspath(build)
    if not os.path.isdir(build):
        run(["cmake", "-S", ROOT, "-B", build, "-DCMAKE_BUILD_TYPE=Release"])
    run(["cmake", "--build", build, "--config", "Release"])
    run(["cpack", "-C", "Release"], cwd=build)
    for name in sorted(os.listdir(build)):
        if name.startswith("Cliderness-") and name.endswith((".zip", ".tar.gz")):
            print("package:", os.path.join(build, name))


def dos():
    src = os.path.join(ROOT, "builds", "dos")
    exes = [e for e in ("CLIDE16.EXE", "CLIDE386.EXE") if os.path.isfile(os.path.join(src, e))]
    if not exes:
        sys.exit("no DOS executables in builds/dos (run builds/dos/build_watcom.sh first)")
    out = os.path.join(ROOT, "dist", "CLIDE")
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    for e in exes:
        shutil.copy2(os.path.join(src, e), out)
    shutil.copytree(os.path.join(ROOT, "data"), os.path.join(out, "DATA"))
    shutil.copy2(os.path.join(ROOT, "LICENSE"), os.path.join(out, "LICENSE.TXT"))
    shutil.copy2(os.path.join(ROOT, "README.md"), os.path.join(out, "README.TXT"))
    shutil.copy2(os.path.join(ROOT, "changelogs.md"), os.path.join(out, "CHANGES.TXT"))
    zpath = os.path.join(ROOT, "dist", "CLIDE-DOS.ZIP")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for base, _dirs, files in os.walk(out):
            for f in files:
                full = os.path.join(base, f)
                z.write(full, os.path.relpath(full, os.path.dirname(out)))
    print("package:", zpath)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--dos":
        dos()
    else:
        native(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build-release"))


if __name__ == "__main__":
    main()
