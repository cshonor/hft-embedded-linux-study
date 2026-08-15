#!/usr/bin/env python3
"""Rename generic *-section.md files to meaningful slugs from headings."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

KEYWORD_SLUGS = [
    ("upvalue", "upvalues"),
    ("流水线", "pipeline"),
    ("小结", "summary"),
    ("总览", "overview-extra"),
    ("能力总览", "capability-summary"),
    ("管线", "pipeline"),
    ("方法论", "methodology"),
    ("布局", "layout"),
    ("协作", "collaboration"),
    ("索引", "index"),
    ("mountain", "mountain"),
    ("编译之山", "compiling-mountain"),
    ("里程碑", "milestone"),
    ("闭包", "closure-extra"),
    ("路径", "call-path"),
    ("对比", "comparison"),
    ("矩阵", "matrix"),
    ("三阶段", "three-stages"),
    ("能力", "capabilities"),
    ("收束", "wrap-up"),
    ("全书", "book-map"),
    ("api", "table-api"),
]


def slug_from_heading(heading: str, seq: int) -> str:
    m = re.search(r"（([^）]+)）", heading)
    if m:
        words = re.findall(r"[A-Za-z0-9]+", m.group(1))
        if words:
            return "-".join(w.lower() for w in words[:5])[:40]
    words = re.findall(r"[A-Za-z0-9]+", heading)
    if words:
        return "-".join(w.lower() for w in words[:5])[:40]
    h = heading.lower()
    for key, slug in KEYWORD_SLUGS:
        if key.lower() in h or key in heading:
            return slug
    return f"extra-{seq:02d}"


def heading_from_file(path: Path) -> str:
    line = path.read_text(encoding="utf-8").splitlines()[0]
    if " · " in line:
        return line.split(" · ", 1)[1].strip()
    return path.stem


def main() -> None:
    renames: dict[str, str] = {}  # old rel -> new rel within chapter
    for chapter in ROOT.rglob("chapter*"):
        if not chapter.is_dir():
            continue
        for f in chapter.glob("*.md"):
            if f.name in ("README.md", "cheat-sheet.md", "00-overview.md"):
                continue
            if re.search(r"-section\.md$", f.name) or f.name == "04-30.md":
                seq = int(f.name[:2])
                heading = heading_from_file(f)
                slug = slug_from_heading(heading, seq)
                new_name = f"{seq:02d}-{slug}.md"
                if (chapter / new_name).exists() and new_name != f.name:
                    new_name = f"{seq:02d}-{slug}-2.md"
                if new_name != f.name:
                    renames[str(f.relative_to(ROOT)).replace("\\", "/")] = str(
                        (chapter / new_name).relative_to(ROOT)
                    ).replace("\\", "/")
                    f.rename(chapter / new_name)

    if not renames:
        print("No renames needed")
        return

    for md in ROOT.rglob("*.md"):
        text = md.read_text(encoding="utf-8")
        new = text
        for old, new_path in renames.items():
            old_name = Path(old).name
            new_name = Path(new_path).name
            new = new.replace(old_name, new_name)
        if new != text:
            md.write_text(new, encoding="utf-8")
    print(f"Renamed {len(renames)} files")


if __name__ == "__main__":
    main()
