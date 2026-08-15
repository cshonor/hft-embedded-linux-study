#!/usr/bin/env python3
"""Restructure Chapter-03-Memory-Ordering to match Chapter-01/02 layout."""
from __future__ import annotations

import shutil
from pathlib import Path

CH3 = Path(__file__).resolve().parents[1] / "Chapter-03-Memory-Ordering"
M = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"


def move_demo(section: str, name: str) -> None:
    src = CH3 / section / name
    dst_dir = CH3 / section / "code"
    dst_dir.mkdir(parents=True, exist_ok=True)
    dst = dst_dir / name
    if src.is_file() and not dst.exists():
        shutil.move(str(src), str(dst))


def move_root_md(name: str, section: str) -> None:
    src = CH3 / name
    dst = CH3 / section / name
    if src.is_file() and not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))


def move_to_33(name: str) -> None:
    src = CH3 / name
    dst = CH3 / "3.3-memory-order-options" / name
    if src.is_file() and not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src), str(dst))


def write(rel: str, text: str) -> None:
    p = CH3 / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + "\n", encoding="utf-8")


def patch(rel: str, pairs: list[tuple[str, str]]) -> None:
    p = CH3 / rel
    if not p.is_file():
        return
    t = p.read_text(encoding="utf-8")
    for old, new in pairs:
        t = t.replace(old, new)
    p.write_text(t, encoding="utf-8", newline="\n")


def main() -> None:
    move_demo("3.3-memory-order-options", "3.3-memory-order-options-demo.rs")
    move_demo("3.4-fences", "3.4-fences-demo.rs")

    for name, sec in [
        ("3.1-reordering-and-optimizations.md", "3.1-reordering-and-optimizations"),
        ("3.2-the-memory-model.md", "3.2-the-memory-model"),
        ("3.3-memory-order-options.md", "3.3-memory-order-options"),
        ("3.4-fences.md", "3.4-fences"),
        ("3.5-common-misconceptions.md", "3.5-common-misconceptions"),
        ("3.6-summary.md", "3.6-summary"),
    ]:
        move_root_md(name, sec)

    for name in [
        "3.3.1-relaxed.md",
        "3.3.2-release-acquire.md",
        "3.3.3-acqrel.md",
        "3.3.4-seq-cst.md",
        "3.3.5-ordering-compare-select.md",
    ]:
        move_to_33(name)

    write(
        "README.md",
        f"""# 第三章

**唯一规则（与第一、二章、实体书一致）**

| 书 § | 笔记 | 代码（有 demo 时） |
|------|------|-------------------|
| 3.Y | `3.Y-english-slug/3.Y-english-slug.md` + 子笔记 `3.Y.Z-*.md` | `3.Y-english-slug/code/*.rs` |

- **章入口**：[本章学习笔记.md](./本章学习笔记.md)（索引表，共 6 节）  
- **贯通**：[Atomics与内存序-贯通笔记.md](../Atomics与内存序-贯通笔记.md)  
- **下一章**：[Chapter-04-Spin-Locks](../Chapter-04-Spin-Locks/本章学习笔记.md)（RA 工程样板）  
- 运行：`cargo build --manifest-path {M}`""",
    )

    write(
        "mod.rs",
        """//! 第三章 — 书 §3.1～3.6 · demo 在 `3.3` / `3.4` 的 `code/`

#[path = "3.4-fences/code/3.4-fences-demo.rs"]
pub mod use_fence;

#[path = "3.3-memory-order-options/code/3.3-memory-order-options-demo.rs"]
pub mod use_seqcst;""",
    )

    # Patch 3.3 sub-notes cross-links
    for name in [
        "3.3.1-relaxed.md",
        "3.3.2-release-acquire.md",
        "3.3.3-acqrel.md",
        "3.3.4-seq-cst.md",
        "3.3.5-ordering-compare-select.md",
    ]:
        patch(
            f"3.3-memory-order-options/{name}",
            [
                ("](./3.2-the-memory-model.md)", "](../3.2-the-memory-model/3.2-the-memory-model.md)"),
                ("](./3.4-fences.md)", "](../3.4-fences/3.4-fences.md)"),
                ("](./3.3-memory-order-options/", "](./code/"),
                ("`atomic/Cargo.toml`", f"`{M}`"),
            ],
        )

    patch(
        "3.3-memory-order-options/3.3-memory-order-options.md",
        [
            ("](./3.3.1-relaxed.md)", "](./3.3.1-relaxed.md)"),
            ("](./3.2-the-memory-model.md)", "](../3.2-the-memory-model/3.2-the-memory-model.md)"),
            ("](./3.4-fences.md)", "](../3.4-fences/3.4-fences.md)"),
            ("./3.3-memory-order-options/3.3-memory-order-options-demo.rs", "./code/3.3-memory-order-options-demo.rs"),
        ],
    )

    for sec, md in [
        ("3.4-fences", "3.4-fences.md"),
        ("3.5-common-misconceptions", "3.5-common-misconceptions.md"),
        ("3.6-summary", "3.6-summary.md"),
    ]:
        patch(
            f"{sec}/{md}",
            [
                ("](./3.3-memory-order-options.md)", "](../3.3-memory-order-options/3.3-memory-order-options.md)"),
                ("](./3.4-fences/", "](./code/"),
                ("](./3.3-memory-order-options/", "](../3.3-memory-order-options/code/"),
                ("](./3.1-reordering-and-optimizations.md", "](../3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md)"),
                ("](./3.4-fences.md)", "](./3.4-fences.md)" if sec != "3.4-fences" else ""),
                ("](./3.5-common-misconceptions.md)", "](../3.5-common-misconceptions/3.5-common-misconceptions.md)"),
                ("](./3.6-summary.md)", "](../3.6-summary/3.6-summary.md)"),
                ("`atomic/Cargo.toml`", f"`{M}`"),
                ("../Chapter-02-Atomics/use_fence.rs", "(已移除；见 mod.rs use_fence)"),
            ],
        )

    print("ch03 file moves + patches done")


if __name__ == "__main__":
    main()
