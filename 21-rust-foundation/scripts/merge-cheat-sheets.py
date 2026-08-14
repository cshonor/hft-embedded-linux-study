#!/usr/bin/env python3
"""Merge *cheat-sheet*.md into parent docs as ## 速记; remove cheat-sheet files and fix links."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {".git", "target", "node_modules", "blanket-trait-demo", "naming-series-demo"}


def find_cheat_sheets() -> list[Path]:
    out: list[Path] = []
    for p in ROOT.rglob("*"):
        if not p.is_file() or p.suffix != ".md":
            continue
        if any(part in SKIP_DIRS for part in p.parts):
            continue
        name = p.name.lower()
        if "cheat-sheet" in name:
            out.append(p)
    return sorted(out)


def resolve_parent(cs: Path) -> Path | None:
    d = cs.parent
    stem = cs.stem  # e.g. "01-cheat-sheet", "cheat-sheet", "03-1-cheat-sheet"

    if stem == "cheat-sheet":
        for candidate in (d / "README.md", d / "00-overview.md"):
            if candidate.is_file():
                return candidate
        # single main topic file
        others = [
            f
            for f in d.glob("*.md")
            if "cheat-sheet" not in f.name.lower()
            and f.name not in ("README.md", "00-overview.md")
        ]
        if len(others) == 1:
            return others[0]
        return d / "README.md" if (d / "README.md").is_file() else None

    # NN-cheat-sheet or NN-M-cheat-sheet
    prefix = stem.replace("-cheat-sheet", "")
    candidates: list[Path] = []
    for f in d.glob("*.md"):
        if "cheat-sheet" in f.name.lower():
            continue
        if f.stem == prefix or f.stem.startswith(prefix + "-"):
            candidates.append(f)
    if not candidates:
        return d / "README.md" if (d / "README.md").is_file() else None
    # prefer longest matching non-README
    candidates.sort(key=lambda p: (p.name == "README.md", -len(p.name)))
    return candidates[0]


def extract_body(cs: Path) -> str:
    text = cs.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    body: list[str] = []
    i = 0
    # skip leading blank + title
    while i < len(lines) and not lines[i].strip():
        i += 1
    if i < len(lines) and lines[i].lstrip().startswith("#"):
        i += 1
    while i < len(lines) and not lines[i].strip():
        i += 1
    # skip ← nav line
    if i < len(lines) and lines[i].strip().startswith("←"):
        i += 1
    while i < len(lines) and not lines[i].strip():
        i += 1
    for line in lines[i:]:
        if line.strip() == "---" and not body:
            continue
        if re.match(r"^#\s+.*(速记|记忆卡片|背诵)", line) and not body:
            continue
        if line.strip().startswith("←") and not body:
            continue
        body.append(line)
    while body and not body[-1].strip():
        body.pop()
    return "\n".join(body).strip()


def has_suji_section(text: str) -> bool:
    return bool(re.search(r"^##\s+速记", text, re.MULTILINE))


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def merge_into_parent(parent: Path, body: str) -> bool:
    if not parent.is_file():
        return False
    text = read_text(parent)
    if has_suji_section(text):
        # append under existing 速记 only if body adds unique content (skip)
        return False
    block = f"\n\n---\n\n## 速记\n\n{body}\n"
    # remove old footer pointing to cheat-sheet
    text = re.sub(
        r"\n→[^\n]*cheat-sheet[^\n]*\n?",
        "\n",
        text,
        flags=re.IGNORECASE,
    )
    text = text.rstrip() + block
    parent.write_text(text + "\n", encoding="utf-8")
    return True


def fix_links_in_file(path: Path) -> bool:
    text = read_text(path)
    orig = text

    # Remove link segments mentioning cheat-sheet
    text = re.sub(
        r"\s*[·\|]\s*\[[^\]]*\]\([^)]*cheat-sheet[^)]*\)",
        "",
        text,
        flags=re.IGNORECASE,
    )
    text = re.sub(
        r"\[[^\]]*\]\([^)]*cheat-sheet[^)]*\)\s*[·\|]?\s*",
        "",
        text,
        flags=re.IGNORECASE,
    )
    # table cells: remove cheat-sheet-only column content
    text = re.sub(
        r"\|\s*—\s*\|\s*(?:背诵提纲|速记)\s*\|",
        "",
        text,
        flags=re.IGNORECASE,
    )
    # standalone lines
    text = re.sub(
        r"^→[^\n]*cheat-sheet[^\n]*\n",
        "",
        text,
        flags=re.IGNORECASE | re.MULTILINE,
    )
    text = re.sub(
        r"^.*cheat-sheet\.md.*\n",
        "",
        text,
        flags=re.IGNORECASE | re.MULTILINE,
    )

    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    sheets = find_cheat_sheets()
    merged = 0
    skipped = 0
    deleted = 0
    no_parent: list[str] = []

    for cs in sheets:
        parent = resolve_parent(cs)
        body = extract_body(cs)
        if parent and body:
            if merge_into_parent(parent, body):
                merged += 1
            else:
                skipped += 1
        elif not parent:
            no_parent.append(str(cs.relative_to(ROOT)))

    # fix all markdown links repo-wide
    fixed = 0
    for md in ROOT.rglob("*.md"):
        if any(part in SKIP_DIRS for part in md.parts):
            continue
        if fix_links_in_file(md):
            fixed += 1

    for cs in sheets:
        if cs.is_file():
            cs.unlink()
            deleted += 1

    print(f"cheat-sheets found: {len(sheets)}")
    print(f"merged into parent: {merged}")
    print(f"skipped (已有速记或无 parent): {skipped}")
    print(f"deleted: {deleted}")
    print(f"files with links fixed: {fixed}")
    if no_parent:
        print("no parent resolved:")
        for p in no_parent[:20]:
            print(f"  {p}")


if __name__ == "__main__":
    main()
