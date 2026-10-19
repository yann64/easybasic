#!/usr/bin/env python3
"""Regenerates docs/architecture/pb_coverage.md.

Compares the full set of real PureBasic procedures (from the locally
installed PureBasic compiler) against this project's own implemented
builtins (compiler/src/sema/sema.cpp's builtin signature tables), grouped
by library.

Requires a local PureBasic install with `pbcompilerc` on PATH (or pass
--pbcompilerc) and its own `purelibraries/` directory next to it (used for
library grouping - PB's own per-library binary definition files).

Usage:
    python3 tools/pb_coverage/generate.py [--pbcompilerc PATH] [--purelibraries PATH]

Run from the repo root; writes docs/architecture/pb_coverage.md directly.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import defaultdict

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SEMA_CPP = os.path.join(REPO_ROOT, "compiler", "src", "sema", "sema.cpp")
OUTPUT_MD = os.path.join(REPO_ROOT, "docs", "architecture", "pb_coverage.md")

# Names found in no purelibraries file at all - compile-time/language-level
# constructs, not real callable library procedures.
LANGUAGE_LEVEL = {"Bool", "Defined", "OffsetOf", "SizeOf", "Subsystem"}

# A handful of names the direct binary scan finds in several files, because
# other libraries merely *reference* them in their own metadata - picked by
# hand to the library that actually defines each.
MULTI_OVERRIDE = {
    "Box": "2ddrawing",
    "Date": "date",
    "DirectoryEntryDate": "filesystem",
    "DirectoryEntrySize": "filesystem",
    "Fog": "world",
    "Yaw": "math3d",
}


def run_listfunctions(pbcompilerc, workdir):
    dummy = os.path.join(workdir, "dummy.pb")
    with open(dummy, "w", encoding="utf-8") as f:
        f.write("a=1\n")
    out = os.path.join(workdir, "functions.txt")
    subprocess.run([pbcompilerc, dummy, "-lf", "-o", out], check=True,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with open(out, encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()
    names = {}
    for line in lines[1:]:  # first line is just a total count
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*(\(.*)$", line)
        if m:
            names[m.group(1)] = m.group(2)
        else:
            name = line.split(" ", 1)[0]
            names[name] = line
    return names


def strings_lines(path):
    out = subprocess.run(["strings", path], capture_output=True, text=True, check=True)
    return out.stdout.splitlines()


def extract_library_funcs(path):
    """A real, public function's own name line is immediately followed by
    its "(args) - desc." line when run through `strings` (default min
    length 4) - overload-resolution-only entries (e.g. AddGadgetItem2/3)
    have no such following line and are skipped, since they aren't
    separate real PB commands."""
    lines = strings_lines(path)
    funcs = {}
    name_re = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
    i = 0
    while i < len(lines) - 1:
        cur, nxt = lines[i], lines[i + 1]
        if name_re.match(cur) and nxt.startswith("("):
            funcs[cur] = nxt
            i += 2
            continue
        i += 1
    return funcs


def find_in_file_binary(data, name):
    """Direct binary scan fallback for names `strings`' min-length-4
    default drops (e.g. Abs/Cos/Box - too short to survive as their own
    isolated string): look for b"<Name>\\x00" followed within a short
    window by '('."""
    pat = name.encode() + b"\x00"
    idx = data.find(pat)
    if idx == -1:
        return False
    window = data[idx + len(pat): idx + len(pat) + 40]
    return b"(" in window


def lib_display_name(path):
    with open(path, "rb") as f:
        data = f.read(64)
    idx = data.find(b"5BIL")
    rest = data[idx + 4:]
    end = rest.find(b"\x00", 8)
    return rest[8:end].decode("latin-1")


def build_library_map(purelibraries_dir, master_names):
    libfiles = sorted(f for f in os.listdir(purelibraries_dir)
                       if os.path.isfile(os.path.join(purelibraries_dir, f)))

    libmap = {}  # name -> lib key
    for lib in libfiles:
        path = os.path.join(purelibraries_dir, lib)
        for fname in extract_library_funcs(path):
            libmap.setdefault(fname, lib)

    missing = [n for n in master_names if n not in libmap and n not in LANGUAGE_LEVEL]
    if missing:
        cache = {}
        for name in missing:
            hits = []
            for lib in libfiles:
                if lib not in cache:
                    with open(os.path.join(purelibraries_dir, lib), "rb") as f:
                        cache[lib] = f.read()
                if find_in_file_binary(cache[lib], name):
                    hits.append(lib)
            if name in MULTI_OVERRIDE:
                libmap[name] = MULTI_OVERRIDE[name]
            elif hits:
                libmap[name] = hits[0]

    display = {}
    for lib in libfiles:
        try:
            display[lib] = lib_display_name(os.path.join(purelibraries_dir, lib))
        except Exception:
            display[lib] = lib

    return libmap, display


def load_implemented():
    with open(SEMA_CPP, encoding="utf-8") as f:
        content = f.read()
    return {m.lower() for m in re.findall(r'\{"([a-z0-9_]+)", TypeSuffix::', content)}


def md_escape(s):
    return s.replace("|", "\\|")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pbcompilerc", default=shutil.which("pbcompilerc") or "pbcompilerc")
    ap.add_argument("--purelibraries", default=None,
                     help="Path to PureBasic's own purelibraries/ dir "
                          "(default: next to --pbcompilerc's own resolved binary)")
    args = ap.parse_args()

    pbcompilerc_path = shutil.which(args.pbcompilerc) or args.pbcompilerc
    if not os.path.isfile(pbcompilerc_path):
        sys.exit(f"pbcompilerc not found at {pbcompilerc_path!r} - pass --pbcompilerc")

    if args.purelibraries:
        purelibraries_dir = args.purelibraries
    else:
        # pbcompilerc lives in <install>/compilers/pbcompilerc
        install_dir = os.path.dirname(os.path.dirname(pbcompilerc_path))
        purelibraries_dir = os.path.join(install_dir, "purelibraries")
    if not os.path.isdir(purelibraries_dir):
        sys.exit(f"purelibraries dir not found at {purelibraries_dir!r} - pass --purelibraries")

    with tempfile.TemporaryDirectory() as workdir:
        master = run_listfunctions(pbcompilerc_path, workdir)

    libmap, display = build_library_map(purelibraries_dir, master.keys())
    implemented = load_implemented()

    groups = defaultdict(list)
    total = 0
    impl_count = 0
    for name, sig in master.items():
        total += 1
        is_impl = name.lower() in implemented
        if is_impl:
            impl_count += 1
        if name in LANGUAGE_LEVEL:
            lib_display = "Language/Compiler"
        else:
            lib_key = libmap.get(name)
            lib_display = display.get(lib_key, lib_key) if lib_key else "UNKNOWN"
        groups[lib_display].append((name, is_impl, sig))

    lines = []
    lines.append("# PureBasic procedure coverage")
    lines.append("")
    lines.append(
        "Generated from the real, locally-installed PureBasic compiler "
        "(`pbcompilerc -lf` for the full procedure list with signatures, "
        "cross-referenced against each library's own internal definition "
        "file under `purebasic/purelibraries/` for library grouping), "
        "compared against this project's own implemented builtins "
        "(`compiler/src/sema/sema.cpp`'s builtin signature tables)."
    )
    lines.append("")
    lines.append(
        f"**Regenerating this table**: `python3 tools/pb_coverage/generate.py` "
        "from the repo root (needs `pbcompilerc` on PATH, or pass "
        "`--pbcompilerc`/`--purelibraries` explicitly) - rewrites this file "
        "directly. Run it after any slice that adds or removes a builtin."
    )
    lines.append("")
    lines.append(f"**Summary**: **{impl_count} / {total}** procedures implemented ({impl_count * 100 // total}%).")
    lines.append("")
    lines.append(f"{len(LANGUAGE_LEVEL)} names (" + ", ".join(f"`{n}`" for n in sorted(LANGUAGE_LEVEL)) + ") are")
    lines.append("compile-time/language-level constructs, not real library procedures - found in no")
    lines.append("library's own definition file at all; listed in their own `Language/Compiler` group.")
    lines.append("")
    lines.append("## By library")
    lines.append("")
    lines.append("| Library | Implemented | Total | % |")
    lines.append("|---|---:|---:|---:|")
    lib_order = sorted(groups.keys(), key=str.lower)
    for lib in lib_order:
        entries = groups[lib]
        impl_n = sum(1 for _, i, _ in entries if i)
        pct = impl_n * 100 // len(entries)
        anchor = re.sub(r"[^a-z0-9]+", "-", lib.lower()).strip("-")
        lines.append(f"| [{md_escape(lib)}](#{anchor}) | {impl_n} | {len(entries)} | {pct}% |")
    lines.append("")

    for lib in lib_order:
        entries = sorted(groups[lib], key=lambda t: t[0].lower())
        impl_n = sum(1 for _, i, _ in entries if i)
        lines.append(f"## {lib}")
        lines.append("")
        lines.append(f"{impl_n}/{len(entries)} implemented.")
        lines.append("")
        lines.append("| Status | Procedure | Signature |")
        lines.append("|---|---|---|")
        for name, is_impl, sig in entries:
            status = "Yes" if is_impl else "No"
            lines.append(f"| {status} | `{md_escape(name)}` | {md_escape(sig)} |")
        lines.append("")

    os.makedirs(os.path.dirname(OUTPUT_MD), exist_ok=True)
    with open(OUTPUT_MD, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {OUTPUT_MD}: {impl_count}/{total} implemented ({impl_count * 100 // total}%)")


if __name__ == "__main__":
    main()
