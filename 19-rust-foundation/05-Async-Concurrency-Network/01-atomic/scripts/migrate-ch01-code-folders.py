#!/usr/bin/env python3
"""Move Ch01 §1.1–1.9 demo *.rs into each section's code/ subfolder; fix paths."""
from __future__ import annotations

import re
import subprocess
from pathlib import Path

ATOMIC = Path(__file__).resolve().parents[1]
CH01 = ATOMIC / "Chapter-01-Rust-Concurrency-Basics"
CH01_REL = "Chapter-01-Rust-Concurrency-Basics"

SECTIONS: dict[str, str] = {
    "1.1": "1.1-threads-in-rust",
    "1.2": "1.2-scoped-threads",
    "1.3": "1.3-shared-ownership",
    "1.4": "1.4-borrowing-data-races",
    "1.5": "1.5-interior-mutability",
    "1.6": "1.6-send-sync",
    "1.7": "1.7-mutex-rwlock",
    "1.8": "1.8-parking-condvar",
    "1.9": "1.9-summary",
}


def git_mv(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists() or not src.is_file():
        return
    subprocess.run(["git", "mv", str(src), str(dst)], check=True, cwd=ATOMIC.parent)


def move_demos_to_code() -> None:
    for slug in SECTIONS.values():
        folder = CH01 / slug
        code_dir = folder / "code"
        code_dir.mkdir(exist_ok=True)
        for rs in sorted(folder.glob("*.rs")):
            git_mv(rs, code_dir / rs.name)


def insert_code_before_demo(text: str) -> str:
    for slug in SECTIONS.values():
        text = re.sub(
            rf"({re.escape(slug)})/(?!code/)([^/\s)]+\.rs)",
            rf"\1/code/\2",
            text,
        )
    return text


def fix_mod_rs() -> None:
    path = CH01 / "mod.rs"
    text = path.read_text(encoding="utf-8")
    text = text.replace(
        "（索引 + 子笔记 + demo）",
        "（索引 + 子笔记 + code/ demo）",
    )
    text = insert_code_before_demo(text)
    path.write_text(text, encoding="utf-8")


def fix_same_section_demo_links(text: str, slug: str) -> str:
    """./foo-demo.rs -> ./code/foo-demo.rs (skip if already ./code/)."""
    return re.sub(
        r"\(\./(?!code/)([^/) ]+\.rs)\)",
        r"(./code/\1)",
        text,
    )


def fix_markdown_files() -> None:
    roots = [ATOMIC, ATOMIC.parent / "rust_network_programming"]
    for root in roots:
        if not root.is_dir():
            continue
        for path in root.rglob("*.md"):
            try:
                text = path.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            if CH01_REL not in text and not str(path).startswith(str(CH01)):
                continue
            new = insert_code_before_demo(text)
            if path.is_relative_to(CH01):
                for slug in SECTIONS.values():
                    if path.is_relative_to(CH01 / slug):
                        new = fix_same_section_demo_links(new, slug)
                        break
            if new != text:
                path.write_text(new, encoding="utf-8")


def main() -> None:
    move_demos_to_code()
    fix_mod_rs()
    fix_markdown_files()
    print("Ch01 code/ migration done.")


if __name__ == "__main__":
    main()
