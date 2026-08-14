#!/usr/bin/env python3
"""Move chapter notes//*.md to chapter root (01-ER convention). Fix relative links."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fix_moved_note_content(text: str) -> str:
    text = text.replace("/notes/", "/")
    # One level shallower: was in notes/ subdir
    text = text.replace("../../../本书目录.md", "../../本书目录.md")
    text = text.replace("../../../../04_Learn-LLVM-17", "../../../04_Learn-LLVM-17")
    text = text.replace("../../../../02_Compiler-Principles", "../../../02_Compiler-Principles")
    text = text.replace("../../../backmatter/", "../../backmatter/")
    text = text.replace("../../../part01_welcome/", "../../part01_welcome/")
    text = text.replace("../../../part02_jlox/", "../../part02_jlox/")
    text = text.replace("../../../part03_clox/", "../../part03_clox/")
    # part README from chapter file (was ../../ from notes)
    text = re.sub(
        r"\(\.\./\.\./README\.md\)",
        r"(../README.md)",
        text,
    )
    return text


def fix_other_md_content(text: str) -> str:
    return text.replace("/notes/", "/")


def main() -> None:
    moved: list[Path] = []
    for notes_dir in sorted(ROOT.rglob("notes")):
        if not notes_dir.is_dir():
            continue
        for note in notes_dir.glob("*.md"):
            dest = notes_dir.parent / note.name
            content = note.read_text(encoding="utf-8")
            dest.write_text(fix_moved_note_content(content), encoding="utf-8")
            note.unlink()
            moved.append(dest)
        if not any(notes_dir.iterdir()):
            notes_dir.rmdir()

    for md in ROOT.rglob("*.md"):
        if md in moved:
            continue
        old = md.read_text(encoding="utf-8")
        new = fix_other_md_content(old)
        if new != old:
            md.write_text(new, encoding="utf-8")

    print(f"Flattened {len(moved)} note files under {ROOT}")


if __name__ == "__main__":
    main()
