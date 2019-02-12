#!/usr/bin/env python3

# Run from Blender's root DIR

SOURCE_DIRS = (
    "source",
    )

USE_MULTIPROCESS = True

replace_all = (
    ("clipsta", "clip_start"),
    ("clipend", "clip_end"),
    ("YF_dofdist", "dof_dist"),
    ("Lamp", "Light"),
)

replace_tables = (
    replace_all,
)


replace_tables_re = [
    [(src, dst) for src, dst in table]
        for table in replace_tables
]


def replace_all(fn, data_src):
    import re

    data_dst = data_src
    for table in replace_tables_re:
        for src_re, dst in table:
            data_dst = re.sub(src_re, dst, data_dst)
    return data_dst


operation = replace_all

import os

def source_files(path):
    for dirpath, dirnames, filenames in os.walk(path):
        dirnames[:] = [d for d in dirnames if not d.startswith(".")]
        for filename in filenames:
            if filename.startswith("."):
                continue
            ext = os.path.splitext(filename)[1]

            # XXX weak, don't touch this!
            if filename.endswith("versioning_dna.c"):
                continue

            # if ext.lower() in {".py"}:
            # if ext.lower() in {".c", ".cc", ".cxx", ".cpp", ".h", ".hxx", ".hpp", ".py"}:
            if ext.lower() in {".c", ".cc", ".cxx", ".cpp", ".h", ".hxx", ".hpp"}:
                yield os.path.join(dirpath, filename)

def operation_wrap(fn):
    with open(fn, "r", encoding="utf-8") as f:
        data_src = f.read()
        data_dst = operation(fn, data_src)

    if data_dst is None or (data_src == data_dst):
        return

    with open(fn, "w", encoding="utf-8") as f:
        f.write(data_dst)


def main():
    if USE_MULTIPROCESS:
        args = [fn for DIR in SOURCE_DIRS for fn in source_files(DIR)]
        import multiprocessing
        job_total = multiprocessing.cpu_count()
        pool = multiprocessing.Pool(processes=job_total * 2)
        pool.map(operation_wrap, args)
    else:
        for fn in source_files(SOURCE_DIR):
            operation_wrap(fn)


if __name__ == "__main__":
    main()
