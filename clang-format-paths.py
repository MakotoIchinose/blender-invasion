#!/usr/bin/env python3

import os
import sys
import subprocess

VERSION_MIN = (6, 0, 0)

# Optionally pass in files to operate on.
paths = sys.argv[1:]
if not paths:
    paths.extend((
        "intern/clog",
        "intern/ghost",
        "intern/guardedalloc",
        "intern/string",
        "source",
    ))

if os.sep != "/":
    paths = [f.replace("/", os.sep) for f in paths]

extensions = (
    ".c", ".cc", ".cpp", ".cxx",
    ".h", ".hh", ".hpp", ".hxx",
    ".m", ".mm",
    ".osl", ".glsl",
)

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
            data = data.expandtabs(4)
        with open(f, 'w', encoding="utf-8") as fh:
            fh.write(data)


def clang_format_version():
    version_output = subprocess.check_output(("clang-format", "-version")).decode('utf-8')
    version = next(iter(v for v in version_output.split() if v[0].isdigit()), None)
    if version is not None:
        version = tuple(int(n) for n in version.split("."))
    return version


def clang_format(files):
    for f in files:
        cmd = (
            "clang-format", "-i", "-verbose", f.encode("ascii")
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
    ]

    convert_tabs_to_spaces(files)
    clang_format(files)


if __name__ == "__main__":
    main()
