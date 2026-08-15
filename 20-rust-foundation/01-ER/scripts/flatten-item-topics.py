#!/usr/bin/env python3
"""Move ER Item topics/*.md up to Item-NN-slug/ and fix links."""
from __future__ import annotations

import re
import shutil
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
ER_ROOT = REPO_ROOT / "ER"


def fix_flattened_body(body: str) -> str:
    """Inverse of split-item-topics fix_topic_links (topics/ -> item root)."""
    body = body.replace("](../README.md)", "](./README.md)")
    body = re.sub(r"\]\(\.\./\.\./(Item-|Chapter-)", r"](../\1", body)
    body = re.sub(r"\]\(\.\./\.\./\.\./\.\./00-Book/", r"](../../../00-Book/", body)
    body = re.sub(r"\]\(\.\./\.\./\.\./\.\./ER-demos/", r"](../../../ER-demos/", body)
    body = re.sub(r"\]\(\.\./\.\./\.\./\.\./ER-", r"](../../../ER-", body)
    return body


def fix_topics_path_in_text(text: str) -> str:
    text = re.sub(r"\(\./topics/", "(./", text)
    text = re.sub(r"\[", "[", text)
    text = re.sub(r"(Item-[\w-]+)/topics/", r"\1/", text)
    return text


def move_topic_files() -> list[Path]:
    moved: list[Path] = []
    for topics_dir in sorted(ER_ROOT.glob("Chapter-*/Item-*/topics")):
        if not topics_dir.is_dir():
            continue
        item_dir = topics_dir.parent
        for src in sorted(topics_dir.glob("*.md")):
            dst = item_dir / src.name
            if dst.exists():
                raise SystemExit(f"target exists: {dst}")
            shutil.move(str(src), str(dst))
            body = dst.read_text(encoding="utf-8")
            dst.write_text(fix_flattened_body(body), encoding="utf-8")
            moved.append(dst)
        if not any(topics_dir.iterdir()):
            topics_dir.rmdir()
    return moved


def patch_all_markdown() -> int:
    count = 0
    for path in REPO_ROOT.rglob("*.md"):
        if ".git" in path.parts:
            continue
        text = path.read_text(encoding="utf-8")
        new = fix_topics_path_in_text(text)
        if new != text:
            path.write_text(new, encoding="utf-8")
            count += 1
    return count


def patch_scripts() -> None:
    for path in ER_ROOT.glob("scripts/*"):
        if path.suffix not in {".py", ".ps1"}:
            continue
        text = path.read_text(encoding="utf-8")
        new = fix_topics_path_in_text(text)
        if new != text:
            path.write_text(new, encoding="utf-8")


def main() -> None:
    moved = move_topic_files()
    patched = patch_all_markdown()
    patch_scripts()
    print(f"moved {len(moved)} topic files")
    print(f"patched {patched} markdown files")
    remaining = list(ER_ROOT.glob("**/topics"))
    if remaining:
        print("WARNING: topics dirs remain:", [str(p) for p in remaining])
    else:
        print("all topics/ directories removed")


if __name__ == "__main__":
    main()
