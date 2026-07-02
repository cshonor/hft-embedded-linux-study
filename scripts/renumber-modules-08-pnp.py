#!/usr/bin/env python3
"""Shift 08-TCP-IP→10 and 10-14 after inserting 08-Practical-Network-Programming."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REPLACEMENTS = [
    ("13-Rust-Quant-Trading-Guide", "14-Rust-Quant-Trading-Guide"),
    ("12-HFT-Low-Latency-Practice", "13-HFT-Low-Latency-Practice"),
    ("11-DPDK-Low-Latency-Network", "12-DPDK-Low-Latency-Network"),
    ("10-Linux-Kernel-Networking", "11-Linux-Kernel-Networking"),
    ("08-TCP-IP-Illustrated-Vol1", "10-TCP-IP-Illustrated-Vol1"),
]

TEXT_REPLACEMENTS = [
    ("`08`–`11`", "`08`–`12`"),
    ("08–11", "08–12"),
    ("`08`→`11`", "`08`→`12`"),
    ("08→09→10→11", "08→09→10→11→12"),
    ("08 → 09 → 10 → 11", "08 → 09 → 10 → 11 → 12"),
    ("`00`–`13`", "`00`–`14`"),
    ("00`–`13`", "00`–`14`"),
    ("`00-` ~ `13-`", "`00-` ~ `14-`"),
    ("封顶（`00`–`13`）", "封顶（`00`–`14`）"),
    ("| **10** | HFT", "| **11** | HFT"),
    ("| **11** | Rust", "| **12** | Rust"),
    ("**12** | HFT Practice", "**13** | HFT Practice"),
    ("**13** | Rust Guide", "**14** | Rust Guide"),
    ("12–13", "13–14"),
    ("`12`/`13`", "`13`/`14`"),
    ("`12` HFT", "`13` HFT"),
    ("`13` Rust", "`14` Rust"),
    ("文件夹 12", "文件夹 13"),
    ("文件夹 13", "文件夹 14"),
    ("与 `12` 映射", "与 `13` 映射"),
    ("12-HFT-Low", "13-HFT-Low"),
    ("13-Rust-Quant", "14-Rust-Quant"),
    ("`11` DPDK", "`12` DPDK"),
    ("`10` Rosen", "`11` Rosen"),
    ("`08` TCP/IP", "`10` TCP/IP"),
    ("08  TCP/IP", "10  TCP/IP"),
    ("09  UNP", "09  UNP"),  # no-op
    ("10  Rosen", "11  Rosen"),
    ("11  DPDK", "12  DPDK"),
    ("12  HFT", "13  HFT"),
    ("13  Rust", "14  Rust"),
    ("| 10 |", "| 10 |"),  # tcpip slot - careful
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "08-Practical-Network-Programming"}


def should_process(path: Path) -> bool:
    if any(p in path.parts for p in SKIP_DIRS):
        return False
    return path.suffix in {".md", ".mdc", ".py"}


def main():
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_process(path):
            continue
        if "renumber-modules" in path.name:
            continue
        text = path.read_text(encoding="utf-8")
        orig = text
        for old, new in REPLACEMENTS:
            text = text.replace(old, new)
        if text != orig:
            path.write_text(text, encoding="utf-8")
            changed += 1
            print(path.relative_to(ROOT))
    print(f"Updated {changed} files")


if __name__ == "__main__":
    main()
