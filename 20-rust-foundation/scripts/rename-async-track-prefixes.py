#!/usr/bin/env python3
"""Rename atomic/async_tokio/rust_network dirs to 01-/02-/03- prefixed names."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASYNC = "05-Async-Concurrency-Network"
EXTS = {".md", ".py", ".rs", ".toml", ".gitignore", ".ps1"}
SKIP = {".git", "target", ".cursor"}

# Longest / most specific first to avoid partial double-replace
REPLACEMENTS = [
    # Full paths from repo root
    (f"{ASYNC}/rust_network_programming/", f"{ASYNC}/03-rust_network_programming/"),
    (f"{ASYNC}/async_tokio/", f"{ASYNC}/02-async_tokio/"),
    (f"{ASYNC}/atomic/", f"{ASYNC}/01-atomic/"),
    # Relative cross-links (deepest first)
    ("../../03-rust_network_programming/", "../../03-rust_network_programming/"),
    ("../../02-async_tokio/", "../../02-async_tokio/"),
    ("../../01-atomic/", "../../01-atomic/"),
    ("../03-rust_network_programming/", "../03-rust_network_programming/"),
    ("../02-async_tokio/", "../02-async_tokio/"),
    ("../01-atomic/", "../01-atomic/"),
    # Backtick prose (after path forms)
    ("`03-rust_network_programming/`", "`03-rust_network_programming/`"),
    ("`02-async_tokio/`", "`02-async_tokio/`"),
    ("`01-atomic/`", "`01-atomic/`"),
    # cd / python from repo root (unprefixed segment after 04-Async)
    (f"cd {ASYNC}/async_tokio/", f"cd {ASYNC}/02-async_tokio/"),
    (f"python {ASYNC}/async_tokio/", f"python {ASYNC}/02-async_tokio/"),
]


def should_process(path: Path) -> bool:
    if path.suffix not in EXTS:
        return False
    return not any(p in SKIP for p in path.parts)


def transform(text: str) -> str:
    for old, new in REPLACEMENTS:
        text = text.replace(old, new)
    return text


def main() -> None:
    n = 0
    for path in ROOT.rglob("*"):
        if not path.is_file() or not should_process(path):
            continue
        if path.name in {"update-paths-after-restructure.py"}:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        new = transform(text)
        if new != text:
            path.write_text(new, encoding="utf-8", newline="\n")
            n += 1
    print(f"Updated {n} files")


if __name__ == "__main__":
    main()
