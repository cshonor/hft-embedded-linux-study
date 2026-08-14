#!/usr/bin/env python3
"""One-time path update after Book/ER/RFR/Rust_Nomicon → 00-Book/01-ER/02-RFR/04-Rust-Nomicon."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Order: longest / most specific first; do not re-run after migration.
REPLACEMENTS = [
    ("Rust_Nomicon/", "04-Rust-Nomicon/"),
    ("Rust_Nomicon\\", "04-Rust-Nomicon\\"),
    ("Rust_Nomicon`", "04-Rust-Nomicon`"),
    ("Book/", "00-Book/"),
    ("Book\\", "00-Book\\"),
    ("RFR/", "02-RFR/"),
    ("RFR\\", "02-RFR\\"),
    # ER last: avoid touching 01-ER, 00-Book, etc.
    ("ER/", "01-ER/"),
    ("ER\\", "01-ER\\"),
]

SKIP_DIRS = {".git", "target", "node_modules"}

TEXT_EXT = {
    ".md", ".toml", ".yml", ".yaml", ".json", ".ps1", ".py", ".rs",
    ".txt", ".mdc", ".sh",
}


def should_process(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return False
    if path.name == "rename-study-dirs.py":
        return False
    return path.suffix.lower() in TEXT_EXT or path.name in {
        "settings.json", "WORKSPACE.md",
    }


def replace_content(text: str) -> str:
    for old, new in REPLACEMENTS:
        text = text.replace(old, new)
    # CI / dependabot bare working-directory
    text = re.sub(r"working-directory:\s*ER\b", "working-directory: 01-ER", text)
    text = re.sub(r"directory:\s*/ER\b", "directory: /01-ER", text)
    return text


def main() -> None:
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_process(path):
            continue
        try:
            raw = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        new = replace_content(raw)
        if new != raw:
            path.write_text(new, encoding="utf-8", newline="\n")
            changed += 1
            print(path.relative_to(ROOT))
    print(f"updated {changed} files")


if __name__ == "__main__":
    main()
