#!/usr/bin/env python3

import os
import sys
import subprocess

CLANG_FORMAT_CMD = "clang-format"
VERSION_MIN = (6, 0, 0)

# Optionally pass in files to operate on.
paths = sys.argv[1:]
if not paths:
    paths.extend((
        "intern/atomic",
        "intern/audaspace",
        "intern/clog",
        "intern/cycles",
        "intern/dualcon",
        "intern/eigen",
        "intern/ffmpeg",
        "intern/ghost",
        "intern/glew-mx",
        "intern/guardedalloc",
        "intern/iksolver",
        "intern/locale",
        "intern/memutil",
        "intern/mikktspace",
        "intern/opencolorio",
        "intern/openvdb",
        "intern/rigidbody",
        "intern/string",
        "intern/utfconv",
        "source",
        "tests/gtests",
    ))

if os.sep != "/":
    paths = [f.replace("/", os.sep) for f in paths]

extensions = (
    ".c", ".cc", ".cpp", ".cxx",
    ".h", ".hh", ".hpp", ".hxx",
    ".m", ".mm",
    ".osl", ".glsl",
)

ignore_files = {
    "intern/cycles/render/sobol.cpp",  # Too heavy for clang-format
}
if os.sep != "/":
    ignore_files = set(f.replace("/", os.sep) for f in ignore_files)

print("Operating on:")
for p in paths:
    print(" ", p)

def source_files_from_git():
    cmd = ("git", "ls-tree", "-r", "HEAD", *paths, "--name-only", "-z")
    files = subprocess.check_output(cmd).split(b'\0')
    return [f.decode('ascii') for f in files]


def convert_tabs_to_spaces(files):
    for f in files:
        print("TabExpand", f)
        with open(f, 'r', encoding="utf-8") as fh:
            data = fh.read()
            if False:
                # Simple 4 space
                data = data.expandtabs(4)
            else:
                # Complex 2 space
                # because some comments have tabs for alignment.
                def handle(l):
                    ls = l.lstrip("\t")
                    d = len(l) - len(ls)
                    if d != 0:
                        return ("  " * d) + ls.expandtabs(4)
                    else:
                        return l.expandtabs(4)

                lines = data.splitlines(keepends=True)
                lines = [handle(l) for l in lines]
                data = "".join(lines)
        with open(f, 'w', encoding="utf-8") as fh:
            fh.write(data)


def clang_format_version():
    version_output = subprocess.check_output((CLANG_FORMAT_CMD, "-version")).decode('utf-8')
    version = next(iter(v for v in version_output.split() if v[0].isdigit()), None)
    if version is not None:
        version = version.split("-")[0]
        version = tuple(int(n) for n in version.split("."))
    return version


def clang_format(files):
    for f in files:
        cmd = (
            CLANG_FORMAT_CMD, "-i", "-verbose", f.encode("ascii")
        )
        subprocess.check_call(cmd)


def main():
    version = clang_format_version()
    if version is None:
        print("Unable to detect 'clang-format -version'")
        sys.exit(1)
    if version < VERSION_MIN:
        print("Version of clang-format is too old:", version, "<", VERSION_MIN)
        sys.exit(1)

    files = [
        f for f in source_files_from_git()
        if f.endswith(extensions)
        if f not in ignore_files
    ]

    convert_tabs_to_spaces(files)
    clang_format(files)


if __name__ == "__main__":
    main()
