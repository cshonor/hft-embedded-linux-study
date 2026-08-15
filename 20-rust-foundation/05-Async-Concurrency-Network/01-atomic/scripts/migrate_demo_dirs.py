# -*- coding: utf-8 -*-
"""Rename demo directories and .rs files to match book section slugs."""
from __future__ import annotations

import re
from pathlib import Path

ATOMIC = Path(__file__).resolve().parent.parent

# (chapter_dir, old_slug, new_slug) — renames demo folder and file prefix old_slug -> new_slug
CHAPTER_RENAMES: list[tuple[str, str, str]] = [
    # Ch01
    ("Chapter-01-Rust-Concurrency-Basics", "1.3-thread-spawn", "1.1-threads-in-rust"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.4-scoped-threads", "1.2-scoped-threads"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.5-shared-ownership", "1.3-shared-ownership"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.6-interior-mutability", "1.5-interior-mutability"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.7-send-sync", "1.6-send-sync"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.8-mutex-rwlock", "1.7-mutex-rwlock"),
    ("Chapter-01-Rust-Concurrency-Basics", "1.9-parking-condvar", "1.8-parking-condvar"),
    # Ch02
    ("Chapter-02-Atomics", "2.2-load-store", "2.1-atomic-load-store"),
    ("Chapter-02-Atomics", "2.3-fetch-modify", "2.2-fetch-and-modify"),
    ("Chapter-02-Atomics", "2.4-cas", "2.3-compare-and-exchange"),
    ("Chapter-02-Atomics", "2.5-quick-demo", "2.4-summary"),
    # Ch03 (move ch2 fence/seqcst demos here)
    ("Chapter-02-Atomics", "2.6-fence", "3.4-fences"),
    ("Chapter-02-Atomics", "2.7-seqcst", "3.3-memory-order-options"),
    # Ch04
    ("Chapter-04-Spin-Locks", "4.1-spin-lock", "4.1-minimal-implementation"),
    # Ch05
    ("Chapter-05-Channels", "5.1-one-shot-channel", "5.2-unsafe-one-shot-channel"),
    # Ch06
    ("Chapter-06-Custom-Arc", "6.1-custom-arc", "6.1-basic-reference-counting"),
]

MOD_REPLACEMENTS: list[tuple[str, str, str]] = []  # filled after renames


def rename_demo_files_in_dir(demo_dir: Path, old_slug: str, new_slug: str) -> None:
    if not demo_dir.is_dir():
        return
    for f in list(demo_dir.iterdir()):
        if f.suffix != ".rs":
            continue
        if f.name.startswith(old_slug):
            new_name = new_slug + f.name[len(old_slug) :]
            f.rename(demo_dir / new_name)
            print(f"  file {f.name} -> {new_name}")


def migrate_slug(chapter: str, old_slug: str, new_slug: str) -> None:
    ch_path = ATOMIC / chapter
    old_dir = ch_path / old_slug
    new_dir = ch_path / new_slug

    if old_slug == new_slug:
        return
    if not old_dir.is_dir():
        print(f"SKIP missing {old_dir}")
        return
    if new_dir.exists():
        print(f"SKIP exists {new_dir}")
        return

    rename_demo_files_in_dir(old_dir, old_slug, new_slug)
    old_dir.rename(new_dir)
    print(f"DIR {chapter}: {old_slug} -> {new_slug}")
    MOD_REPLACEMENTS.append((chapter, old_slug, new_slug))


def patch_mod_rs(chapter: str) -> None:
    mod_path = ATOMIC / chapter / "mod.rs"
    if not mod_path.exists():
        return
    text = mod_path.read_text(encoding="utf-8")
    for ch, old_slug, new_slug in MOD_REPLACEMENTS:
        if ch != chapter:
            continue
        text = text.replace(f'"{old_slug}/', f'"{new_slug}/')
        text = text.replace(f"{old_slug}-", f"{new_slug}-")
    mod_path.write_text(text, encoding="utf-8")
    print(f"Patched {mod_path.name} in {chapter}")


def move_ch2_fence_seqcst_to_ch3() -> None:
    """After rename, 3.4-fences and 3.3-memory-order-options may live under Ch02 — move to Ch03."""
    ch2 = ATOMIC / "Chapter-02-Atomics"
    ch3 = ATOMIC / "Chapter-03-Memory-Ordering"
    for slug in ("3.4-fences", "3.3-memory-order-options"):
        src = ch2 / slug
        dst = ch3 / slug
        if src.is_dir() and not dst.exists():
            src.rename(dst)
            print(f"MOVED to Ch03: {slug}")


def main():
    global MOD_REPLACEMENTS
    MOD_REPLACEMENTS = []
    for chapter, old_slug, new_slug in CHAPTER_RENAMES:
        migrate_slug(chapter, old_slug, new_slug)
    move_ch2_fence_seqcst_to_ch3()
    chapters = {c for c, _, _ in CHAPTER_REPLACEMENTS}
    chapters.add("Chapter-03-Memory-Ordering")
    for ch in chapters:
        patch_mod_rs(ch)
    # Ch03 mod if exists
    ch3_mod = ATOMIC / "Chapter-03-Memory-Ordering" / "mod.rs"
    if not ch3_mod.exists() and (ATOMIC / "Chapter-03-Memory-Ordering" / "3.4-fences").exists():
        create_ch03_mod()
    print("Migration done.")


def create_ch03_mod() -> None:
    content = '''//! 第三章 — 内存序 demo（fence / seqcst）

#[path = "3.4-fences/3.4-fences-demo.rs"]
pub mod use_fence;

#[path = "3.3-memory-order-options/3.3-memory-order-options-seq_cst-demo.rs"]
pub mod use_seqcst;
'''
    path = ATOMIC / "Chapter-03-Memory-Ordering" / "mod.rs"
    path.write_text(content, encoding="utf-8")
    src_mod = ATOMIC / "src" / "mod.rs"
    text = src_mod.read_text(encoding="utf-8")
    if "chapter_03" not in text:
        text = text.replace(
            "#[path = \"../Chapter-02-Atomics/mod.rs\"]\npub mod chapter_02;",
            "#[path = \"../Chapter-02-Atomics/mod.rs\"]\npub mod chapter_02;\n\n#[path = \"../Chapter-03-Memory-Ordering/mod.rs\"]\npub mod chapter_03;",
        )
        src_mod.write_text(text, encoding="utf-8")
    print("Created Chapter-03 mod.rs")


if __name__ == "__main__":
    main()
