#!/usr/bin/env python3
"""Path updates after:
  03-Rust_Nomicon → 04-Rust-Nomicon
  04-Async-Concurrency-Network → 05-Async-Concurrency-Network
  05_Compilers-and-LLVM-Learning → 06_Compilers-and-LLVM-Learning
  new 03-DeepRustStdLib
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {".git", "target", "node_modules", ".cursor"}
TEXT_EXT = {
    ".md", ".toml", ".yml", ".yaml", ".json", ".ps1", ".py", ".rs",
    ".txt", ".mdc", ".sh", ".gitignore",
}

# Longest / most specific first — single pass each
REPLACEMENTS = [
    ("05_Compilers-and-LLVM-Learning", "06_Compilers-and-LLVM-Learning"),
    ("04-Async-Concurrency-Network", "05-Async-Concurrency-Network"),
    ("03-Rust_Nomicon", "04-Rust-Nomicon"),
    # Learning-path shorthand (after path renames)
    ("Rust **04 / 05**", "Rust **05 / 06**"),
    ("④ Async", "⑤ Async"),
    ("④ atomic", "⑤ atomic"),
    ("④ 实战", "⑤ 实战"),
    ("④ 的 Rust", "⑤ 的 Rust"),
    ("④ 专题", "⑤ 专题"),
    ("04 专题", "05 专题"),
    ("04 在 Nomicon", "05 在 Nomicon"),
    ("04(01→02→03)", "05(01→02→03)"),
    ("→ 04(", "→ 05("),
    ("⑤a C++", "⑥a C++"),
    ("⑤b Compilers", "⑥b Compilers"),
    ("⑤b Learn", "⑥b Learn"),
    ("05 LLVM", "06 LLVM"),
    ("05 现在", "06 现在"),
    ("05(LLVM/IR)", "06(LLVM/IR)"),
    ("编译器专题（05 ·", "编译器专题（06 ·"),
    ("| **05** |", "| **06** |"),
    ("| **5b** |", "| **6b** |"),
    ("| **5a** |", "| **6a** |"),
    ("| **4** | 并发", "| **5** | 并发"),
    ("③ Rust Nomicon", "④ Rust Nomicon"),
    ("③ 03-Rust_Nomicon", "④ 04-Rust-Nomicon"),
    ("③ 03-Rust_Nomicon", "④ 04-Rust-Nomicon"),
    ("③ Rust Nomicon（", "④ Rust Nomicon（"),
    ("→  ③ Rust Nomicon", "→  ④ Rust Nomicon"),
    ("→ ③ 03-Rust_Nomicon", "→ ④ 04-Rust-Nomicon"),
    ("RFR → ER → Nomicon → 04", "RFR → ER → DeepRustStdLib → Nomicon → 05"),
    ("`05/README`", "`06/README`"),
    ("详见 [`05/README`]", "详见 [`06/README`]"),
    ("见 [`05/README`]", "见 [`06/README`]"),
    ("→ [`05/README`]", "→ [`06/README`]"),
    ("→ [`05_Compilers", "→ [`06_Compilers"),
    ("../../05_Compilers", "../../06_Compilers"),
    ("../05_Compilers", "../06_Compilers"),
]


def should_process(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return False
    if path.name == "restructure-03-stdlib-nomicon-shift.py":
        return False
    if path.suffix.lower() in TEXT_EXT:
        return True
    return path.name in {"settings.json", "WORKSPACE.md", ".gitignore"}


def main() -> None:
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_process(path):
            continue
        try:
            raw = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        new = raw
        for old, repl in REPLACEMENTS:
            new = new.replace(old, repl)
        if new != raw:
            path.write_text(new, encoding="utf-8", newline="\n")
            changed += 1
            print(path.relative_to(ROOT))
    print(f"updated {changed} files")


if __name__ == "__main__":
    main()
