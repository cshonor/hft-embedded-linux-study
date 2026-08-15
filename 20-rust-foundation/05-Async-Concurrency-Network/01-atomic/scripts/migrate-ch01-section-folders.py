#!/usr/bin/env python3
"""Move Ch01 §1.1–1.9 notes into per-section folders; fix markdown links."""
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
    if dst.exists():
        return
    subprocess.run(["git", "mv", str(src), str(dst)], check=True, cwd=ATOMIC.parent)


def move_files() -> None:
    for prefix, slug in SECTIONS.items():
        folder = CH01 / slug
        folder.mkdir(exist_ok=True)

        index = CH01 / f"{slug}.md"
        if index.is_file():
            git_mv(index, folder / f"{slug}.md")

        for path in sorted(CH01.glob(f"{prefix}.*.md")):
            git_mv(path, folder / path.name)

        bak = CH01 / f"{slug}.full.bak.md"
        if bak.is_file():
            git_mv(bak, folder / bak.name)


def fix_global_paths(text: str) -> str:
    for prefix, slug in SECTIONS.items():
        text = re.sub(
            rf"({re.escape(CH01_REL)}/)(?!{re.escape(slug)}/){re.escape(prefix)}\.",
            lambda m, s=slug, p=prefix: f"{m.group(1)}{s}/{p}.",
            text,
        )
        text = re.sub(
            rf"({re.escape(CH01_REL)}/)(?!{re.escape(slug)}/){re.escape(slug)}\.md",
            lambda m, s=slug: f"{m.group(1)}{s}/{s}.md",
            text,
        )
        text = re.sub(
            rf"({re.escape(CH01_REL)}/)(?!{re.escape(slug)}/){re.escape(slug)}/",
            lambda m, s=slug: f"{m.group(1)}{s}/",
            text,
        )
    return text


def fix_in_section_file(text: str, slug: str, prefix: str) -> str:
    # demo paths: ./1.1-threads-in-rust/xxx.rs -> ./xxx.rs（勿匹配 ../1.1-threads-in-rust/）
    text = re.sub(rf"(?<!\.\.)\./{re.escape(slug)}/", "./", text)

    for other_prefix, other_slug in SECTIONS.items():
        if other_prefix == prefix:
            continue
        # cross-section index
        text = re.sub(
            rf"\./{re.escape(other_slug)}\.md",
            f"../{other_slug}/{other_slug}.md",
            text,
        )
        # cross-section sub-notes ./1.2.4-...
        text = re.sub(
            rf"\./({re.escape(other_prefix)}\.[0-9]+[^)\s]*\.md)",
            lambda m, s=other_slug: f"../{s}/{m.group(1)}",
            text,
        )
        # cross-section demo dir
        text = re.sub(
            rf"\./{re.escape(other_slug)}/",
            f"../{other_slug}/",
            text,
        )

    text = text.replace("./本章学习笔记.md", "../本章学习笔记.md")
    return text


def fix_chapter_index(text: str) -> str:
    for prefix, slug in SECTIONS.items():
        text = re.sub(
            rf"\(\./({re.escape(prefix)}\.[0-9]+[^)]*\.md)\)",
            lambda m, s=slug: f"(./{s}/{m.group(1)})",
            text,
        )
        text = re.sub(
            rf"\(\./({re.escape(slug)}\.md)\)",
            lambda m, s=slug: f"(./{s}/{m.group(1)})",
            text,
        )
    return text


def fix_broken_relative_paths(text: str) -> str:
    text = re.sub(r"\(\.\.\./", "(../", text)
    return text


def update_markdown() -> None:
    repo_root = ATOMIC.parent
    for md in repo_root.rglob("*.md"):
        if "node_modules" in md.parts:
            continue
        text = md.read_text(encoding="utf-8")
        orig = text
        text = fix_global_paths(text)
        text = fix_broken_relative_paths(text)

        rel = md.relative_to(CH01) if md.is_relative_to(CH01) else None
        if rel and len(rel.parts) >= 2:
            slug = rel.parts[0]
            prefix = next((p for p, s in SECTIONS.items() if s == slug), None)
            if prefix:
                text = fix_in_section_file(text, slug, prefix)
                text = fix_broken_relative_paths(text)

        if md.name == "本章学习笔记.md" and md.parent == CH01:
            text = fix_chapter_index(text)

        if text != orig:
            md.write_text(text, encoding="utf-8")


def update_spec_doc() -> None:
    spec = ATOMIC / "小节笔记与Demo规范.md"
    if not spec.is_file():
        return
    text = spec.read_text(encoding="utf-8")
    old_tree = """```
Chapter-01-Rust-Concurrency-Basics/
├── 本章学习笔记.md              ← 索引表
├── 1.1-threads-in-rust.md       ← §1.1 正文
├── 1.1-threads-in-rust/
│   └── 1.1-threads-in-rust-join-demo.rs
├── 1.7-mutex-rwlock.md
├── 1.7-mutex-rwlock/
│   └── 1.7-mutex-rwlock-mutex-demo.rs
└── mod.rs
```"""
    new_tree = """```
Chapter-01-Rust-Concurrency-Basics/
├── 本章学习笔记.md
├── mod.rs
├── 1.1-threads-in-rust/
│   ├── 1.1-threads-in-rust.md   ← §1.1 索引
│   ├── 1.1.1-spawn-join-basics.md
│   └── 1.1-threads-in-rust-join-demo.rs
├── 1.7-mutex-rwlock/
│   ├── 1.7-mutex-rwlock.md
│   ├── 1.7.1-mutex-motivation.md
│   └── 1.7-mutex-rwlock-mutex-demo.rs
└── …（§1.2～1.9 同结构：每节一文件夹，索引 + 子笔记 + demo）
```"""
    if old_tree in text:
        text = text.replace(old_tree, new_tree)
        text = text.replace(
            "| `1.Y-english-slug.md` | **该节正文**",
            "| `1.Y-english-slug/1.Y-english-slug.md` | **该节索引**；子笔记 `1.Y.Z-*.md` 同目录",
        )
        spec.write_text(text, encoding="utf-8")


def main() -> None:
    move_files()
    update_markdown()
    update_spec_doc()
    print("Ch01 section folder migration done.")


if __name__ == "__main__":
    main()
