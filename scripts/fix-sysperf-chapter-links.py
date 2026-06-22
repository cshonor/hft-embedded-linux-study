#!/usr/bin/env python3
"""Fix chapter-XX.md links after SysPerf folder restructure."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SYSPERF = ROOT / "02-Systems-Performance-2nd"

CHAPTER_LINK = re.compile(
    r"(?<!\./\./)(?<!\.\./\.\./)(\./)?(chapter-\d{2}-[^)\s#/]+\.md)"
)


def fix_content(text: str, depth: int) -> str:
    """depth: 0=syperf root, 1=chapter README, 2=notes/"""
    def repl(m: re.Match) -> str:
        path = m.group(2)
        folder = path[:-3] + "/"
        if depth == 0:
            return "./" + folder
        if depth == 1:
            return "../" + folder
        if depth == 2:
            return "../../" + folder
        return folder

    # chapter links
    text = re.sub(
        r"\(\./(chapter-\d{2}-[^)\s#/]+\.md)(#[^)]+)?\)",
        lambda m: f"({'../' if depth == 1 else '../../' if depth == 2 else './'}{m.group(1)[:-3]}/{m.group(2) or ''})".replace("/#", "/notes/"),  # anchors simplified
        text,
    )
    # simpler: replace chapter-XX-yyy.md with path prefix
    def sub_path(m):
        fname = m.group(0)
        folder = fname.replace(".md", "/")
        if depth == 1:
            if fname.startswith("./"):
                return "../" + folder[2:]
            return "../" + folder
        if depth == 2:
            return "../../" + folder.lstrip("./")
        return "./" + folder.lstrip("./")

    text = re.sub(r"\.?/?chapter-\d{2}-[^\s)\]#/]+\.md", sub_path, text)

    # appendix / OUTLINE from chapter depth
    if depth >= 1:
        text = re.sub(r"\(\./(OUTLINE\.md)\)", r"(../\1)", text)
        text = re.sub(r"\(\./(appendix-[^)]+\.md)\)", r"(../\1)", text)
    if depth >= 2:
        text = re.sub(r"\(\.\./(OUTLINE\.md)\)", r"(../../\1)", text)
        text = re.sub(r"\(\.\./(appendix-[^)]+\.md)\)", r"(../../\1)", text)

    return text


def process_file(path: Path):
    rel = path.relative_to(SYSPERF)
    parts = rel.parts
    if parts[0].startswith("appendix"):
        depth = 0
    elif len(parts) == 1:
        depth = 0
    elif len(parts) == 2 and parts[1] == "README.md":
        depth = 1
    elif len(parts) >= 3 and parts[1].startswith("chapter") and parts[2] == "notes":
        depth = 2
    elif parts[0].startswith("chapter"):
        depth = 1
    else:
        depth = 0

    old = path.read_text(encoding="utf-8")
    new = fix_content(old, depth)
    if new != old:
        path.write_text(new, encoding="utf-8")
        print(f"fixed {rel}")


def main():
    for path in SYSPERF.rglob("*.md"):
        process_file(path)
    # Fix repo-wide links that explicitly point at SysPerf flat chapter files
    pattern = re.compile(
        r"02-Systems-Performance-2nd/(chapter-\d{2}-[^)\s#/]+\.md)"
    )
    for path in ROOT.rglob("*.md"):
        if SYSPERF in path.parents or path.parent == SYSPERF:
            continue
        old = path.read_text(encoding="utf-8")
        new = pattern.sub(
            lambda m: f"02-Systems-Performance-2nd/{m.group(1)[:-3]}/",
            old,
        )
        if new != old:
            path.write_text(new, encoding="utf-8")
            print(f"fixed external {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
