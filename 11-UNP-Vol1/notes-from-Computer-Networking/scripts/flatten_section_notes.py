#!/usr/bin/env python3
"""Lift section notes.md / study.md to chapter dir as {section}.md; fix links; relocate code/."""

from __future__ import annotations

import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGETS = [
    ROOT / "UNP_Vol1",
    ROOT / "TCP-IP-Volume1-Protocols",
]

# Section folder names: 3.2_Foo_Bar or 1.1-architecture-principles
SECTION_DIR_RE = re.compile(r"^\d+\.\d+([_-].+)?$")

# Links: .../3.2_SocketAddressStructure/notes.md -> .../3.2_SocketAddressStructure.md
LINK_FIX_RE = re.compile(
    r"([0-9]+\.[0-9]+[-_A-Za-z0-9]+)/(notes|study)\.md"
)

CHAPTER_DIR_RE = re.compile(r"^(Chapter\d+_|chapter\d+-)")


def is_chapter_dir(p: Path) -> bool:
    return CHAPTER_DIR_RE.match(p.name) is not None


def is_section_dir(p: Path) -> bool:
    return p.is_dir() and SECTION_DIR_RE.match(p.name) is not None


def flatten_chapter(chapter: Path) -> list[tuple[Path, Path]]:
    """Return list of (old_path, new_path) moves."""
    moves: list[tuple[Path, Path]] = []
    for section in sorted(chapter.iterdir()):
        if not is_section_dir(section):
            continue
        dest_md = chapter / f"{section.name}.md"
        for name in ("notes.md", "study.md"):
            src = section / name
            if src.exists() and not dest_md.exists():
                moves.append((src, dest_md))
                break
            if src.exists() and dest_md.exists():
                # prefer notes.md content; merge skip — keep existing dest
                moves.append((src, dest_md))
                break
        code_src = section / "code"
        if code_src.is_dir():
            code_dest = chapter / "code" / section.name
            if not code_dest.exists():
                moves.append((code_src, code_dest))
    return moves


def remove_empty_sections(chapter: Path) -> None:
    for section in list(chapter.iterdir()):
        if not is_section_dir(section):
            continue
        # remove leftover files/dirs
        for child in list(section.iterdir()):
            if child.name == ".gitkeep":
                child.unlink(missing_ok=True)
            elif child.is_dir():
                shutil.rmtree(child, ignore_errors=True)
            else:
                child.unlink(missing_ok=True)
        try:
            section.rmdir()
        except OSError:
            pass


def apply_moves(moves: list[tuple[Path, Path]]) -> None:
    for src, dest in moves:
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.exists() and src != dest:
            dest.unlink()
        shutil.move(str(src), str(dest))


def fix_markdown_links(root: Path) -> int:
    n = 0
    for md in root.rglob("*.md"):
        text = md.read_text(encoding="utf-8")
        new, count = LINK_FIX_RE.subn(r"\1.md", text)
        # study.md index lines
        new2, c2 = re.subn(
            r"`(\d+\.x_[^`]+)/notes\.md`",
            r"`\1.md`",
            new,
        )
        new3, c3 = re.subn(
            r"逐节[：:]\s*`[^`]*notes\.md`",
            "逐节：同目录下 `节名.md`",
            new2,
        )
        total = count + c2 + c3
        if total:
            md.write_text(new3, encoding="utf-8")
            n += total
    return n


def update_study_index_lines(chapter: Path) -> None:
    study = chapter / "study.md"
    if not study.exists():
        return
    text = study.read_text(encoding="utf-8")
    # ./10.1_Overview/notes.md or [10.1](./10.1_Overview/notes.md)
    text = LINK_FIX_RE.sub(r"\1.md", text)
    text = re.sub(
        r"\]\(\./([^/)]+)/notes\.md\)",
        r"](\1.md)",
        text,
    )
    text = re.sub(
        r"\]\(\./([^/)]+)/study\.md\)",
        r"](\1.md)",
        text,
    )
    # build simple file list for tcpip style [1.1](1.1-foo/study.md)
    if "逐节" in text and "notes.md" not in text:
        pass
    study.write_text(text, encoding="utf-8")


def process_tree(root: Path) -> dict[str, int]:
    stats = {"moves": 0, "sections_removed": 0, "link_fixes": 0}
    all_moves: list[tuple[Path, Path]] = []
    for chapter in sorted(root.rglob("*")):
        if not chapter.is_dir() or not is_chapter_dir(chapter):
            continue
        # only direct chapter folders under phase or chapterNN-* top
        if chapter.parent.name in ("UNP_Vol1", "TCP-IP-Volume1-Protocols"):
            pass
        elif not any(
            p.name.startswith(("1_", "2_", "3_", "4_"))
            or CHAPTER_DIR_RE.match(p.name)
            for p in [chapter.parent]
        ):
            # chapter under 1_BasicFoundation etc.
            if not re.match(r"^\d+_", chapter.parent.name):
                if chapter.parent.name not in (
                    "1_BasicFoundation",
                    "2_AdvancedSkill",
                    "3_DeepMaster",
                    "4_ArchitectureDesign",
                ):
                    continue
        all_moves.extend(flatten_chapter(chapter))

    apply_moves(all_moves)
    stats["moves"] = len(all_moves)

    for chapter in sorted(root.rglob("*")):
        if chapter.is_dir() and is_chapter_dir(chapter):
            before = sum(1 for _ in chapter.iterdir() if is_section_dir(_))
            remove_empty_sections(chapter)
            after = sum(1 for _ in chapter.iterdir() if is_section_dir(_))
            stats["sections_removed"] += before - after
            update_study_index_lines(chapter)

    stats["link_fixes"] = fix_markdown_links(root)
    return stats


def main() -> None:
    for tree in TARGETS:
        if not tree.exists():
            print(f"skip missing {tree}")
            continue
        print(f"\n=== {tree.name} ===")
        s = process_tree(tree)
        print(f"  moved: {s['moves']}, sections removed: {s['sections_removed']}, link subs: {s['link_fixes']}")


if __name__ == "__main__":
    main()
