#!/usr/bin/env python3

import os
import sys
import subprocess

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

    # could use clang-format on/off
    "source/blender/blenlib/BLI_compiler_typecheck.h",    # Over wraps args.
    "source/blender/blenlib/BLI_utildefines_variadic.h",  # Over wraps args.
    "tests/gtests/blenlib/BLI_ressource_strings.h",       # Large data file, not helpful to format.


    "source/blender/blenlib/intern/sort.c",               # Copied from FreeBSD, don't reformat.
    "source/blender/blenlib/intern/fnmatch.c",            # Copied from GNU libc, don't reformat.
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
            data = data.expandtabs(4)
        with open(f, 'w', encoding="utf-8") as fh:
            fh.write(data)


def clang_format_version():
    version_output = subprocess.check_output(("clang-format", "-version")).decode('utf-8')
    version = next(iter(v for v in version_output.split() if v[0].isdigit()), None)
    if version is not None:
        version = version.split("-")[0]
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
        if f not in ignore_files
    ]

    convert_tabs_to_spaces(files)
    clang_format(files)


if __name__ == "__main__":
    main()
