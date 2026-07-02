#!/usr/bin/env python3
"""Move ULK to 08; shift old 08-15 → 09-16. Run from repo root."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# git mv: high → low, stash ULK first
GIT_MV = [
    ("16-Understanding-Linux-Kernel", "_tmp-08-ULK"),
    ("15-Rust-Quant-Trading-Guide", "16-Rust-Quant-Trading-Guide"),
    ("14-HFT-Low-Latency-Practice", "15-HFT-Low-Latency-Practice"),
    ("13-DPDK-Low-Latency-Network", "12-DPDK-Low-Latency-Network"),
    ("12-Linux-Kernel-Networking", "11-Linux-Kernel-Networking"),
    ("11-TCP-IP-Illustrated-Vol1", "10-TCP-IP-Illustrated-Vol1"),
    ("10-UNP-Vol1", "09-UNP-Vol1"),
    ("09-Practical-Network-Programming", "08-Practical-Network-Programming"),
    ("08-system-low-level-hands-on", "07-system-low-level-hands-on"),
    ("_tmp-08-ULK", "08-Understanding-Linux-Kernel"),
]

# Apply high→low to avoid partial overlaps
TEXT_REPLACEMENTS = [
    ("15-Rust-Quant-Trading-Guide", "16-Rust-Quant-Trading-Guide"),
    ("14-HFT-Low-Latency-Practice", "15-HFT-Low-Latency-Practice"),
    ("13-DPDK-Low-Latency-Network", "12-DPDK-Low-Latency-Network"),
    ("12-Linux-Kernel-Networking", "11-Linux-Kernel-Networking"),
    ("11-TCP-IP-Illustrated-Vol1", "10-TCP-IP-Illustrated-Vol1"),
    ("10-UNP-Vol1", "09-UNP-Vol1"),
    ("09-Practical-Network-Programming", "08-Practical-Network-Programming"),
    ("08-system-low-level-hands-on", "07-system-low-level-hands-on"),
    ("16-Understanding-Linux-Kernel", "08-Understanding-Linux-Kernel"),
    # prose / chain (after path swaps)
    ("05 → 16 ULK", "05 → 08 ULK"),
    ("05 → 16 → 06", "05 → 08 → 06"),
    ("`16 ULK`", "`08 ULK`"),
    ("16 ULK（", "08 ULK（"),
    ("| **16** |", "| **06** |"),
    ("文件夹 `00`–`15`", "文件夹 `00`–`16`"),
    ("`00 → 01(+04) → 02 → 03 → 05 → 16 → 06 → 07 → 08 → 09 → 10 → 01网络章 → 11 → 12 → 13 → 14 → 15`",
     "`00 → 01(+04) → 02 → 03 → 05 → 08 → 06 → 07 → 09 → 10 → 11 → 01网络章 → 12 → 13 → 14 → 15 → 16`"),
    ("05–08         09–13           14–15", "03–06         07–12           15–16"),
    ("08  自制 OS / CPU", "09  自制 OS / CPU"),
    ("09  陈硕 PNP", "10  陈硕 PNP"),
    ("10  UNP", "11  UNP"),
    ("11  TCP/IP → 12  Rosen → 13  DPDK", "12  TCP/IP → 13  Rosen → 14  DPDK"),
    ("14  HFT Practice", "15  HFT Practice"),
    ("15  Rust Guide", "16  Rust Guide"),
    ("**14–15**", "**15–16**"),
    ("| **14–15** |", "| **15–16** |"),
    ("`14`/`15`", "`15`/`16`"),
    ("→ `08` 自制系统", "→ `07` 自制系统"),
    ("→ **`08` 本模块**", "→ **`07` 本模块**"),
    ("`07` TLPI → **`08` 本模块**", "`07` TLPI → **`07` 本模块**"),
    ("`08` 自制系统", "`07` 自制系统"),
    ("**文件夹 `08`**", "**文件夹 `09`**"),
    ("| 14 |", "| 15 |"),
    ("| 15 |", "| 16 |"),
    ("[11 HFT](./14-HFT", "[15 HFT](./15-HFT"),
    ("[12 Rust](./15-Rust", "[16 Rust](./16-Rust"),
    ("| 11 / 12 |", "| 15 / 16 |"),
    ("`05`–`11` 系统纵深", "`03`–`06` 系统纵深"),
    ("[12-HFT](../14-HFT", "[15-HFT](../15-HFT"),
    ("[13-Rust](../15-Rust", "[16-Rust](../16-Rust"),
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "__pycache__"}
SKIP_FILES = {"renumber-modules-08-ulk.py", "renumber-modules-07-tlpi.py"}


def git_mv(src: str, dst: str) -> None:
    r = subprocess.run(["git", "mv", src, dst], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"git mv failed: {src} -> {dst}\n{r.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"git mv {src} -> {dst}")


def update_links() -> int:
    n = 0
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix not in {".md", ".mdc", ".py"}:
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        if path.name in SKIP_FILES:
            continue
        text = path.read_text(encoding="utf-8")
        orig = text
        for old, new in TEXT_REPLACEMENTS:
            text = text.replace(old, new)
        if text != orig:
            path.write_text(text, encoding="utf-8")
            n += 1
    return n


def main() -> None:
    for src, dst in GIT_MV:
        if not (ROOT / src).exists() and src.startswith("_tmp"):
            continue
        git_mv(src, dst)
    n = update_links()
    print(f"Updated {n} text files")


if __name__ == "__main__":
    main()
