#!/usr/bin/env python3
"""Restructure Chapter-02-Atomics to match Chapter-01 layout."""
from __future__ import annotations

import shutil
from pathlib import Path

CH2 = Path(__file__).resolve().parents[1] / "Chapter-02-Atomics"
MANIFEST = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"


def move_demo(section: str, name: str) -> None:
    src = CH2 / section / name
    dst_dir = CH2 / section / "code"
    dst_dir.mkdir(parents=True, exist_ok=True)
    dst = dst_dir / name
    if src.is_file() and not dst.exists():
        shutil.move(str(src), str(dst))


def write(rel: str, text: str) -> None:
    p = CH2 / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + "\n", encoding="utf-8")


def main() -> None:
    for sec, demos in {
        "2.1-atomic-load-store": [
            "2.1-atomic-load-store-demo.rs",
            "2.1-atomic-load-store-lazy-init-demo.rs",
        ],
        "2.2-fetch-and-modify": ["2.2-fetch-and-modify-demo.rs"],
        "2.3-compare-and-exchange": ["2.3-compare-and-exchange-id-allocator-demo.rs"],
        "2.4-summary": ["2.4-summary-demo.rs"],
    }.items():
        for d in demos:
            move_demo(sec, d)

    # Remove old root-level section md (replaced by folder index)
    for name in [
        "2.1-atomic-load-store.md",
        "2.2-fetch-and-modify.md",
        "2.3-compare-and-exchange.md",
        "2.4-summary.md",
    ]:
        p = CH2 / name
        if p.is_file():
            p.unlink()

    write(
        "README.md",
        f"""# 第二章

**唯一规则（与第一章、实体书一致）**

| 书 § | 笔记 | 代码（有 demo 时） |
|------|------|-------------------|
| 2.Y | `2.Y-english-slug/2.Y-english-slug.md` + 子笔记 `2.Y.Z-*.md` | `2.Y-english-slug/code/*.rs` |

- **章入口**：[本章学习笔记.md](./本章学习笔记.md)（索引表，共 4 节）  
- **贯通**：[Atomics与内存序-贯通笔记.md](../Atomics与内存序-贯通笔记.md) · [CAS与Fetch-Modify专题.md](./CAS与Fetch-Modify专题.md)  
- **内存序 demo** 在第 3 章：`3.3-memory-order-options/`、`3.4-fences/`  
- 运行：`cargo build --manifest-path {MANIFEST}`""",
    )

    write(
        "本章学习笔记.md",
        f"""# 第二章 — 学习笔记索引（实体书 §2.1～2.4）

> **结构**：书 § = 文件夹名（如 `2.1-atomic-load-store/`）；每节含 **索引**、子笔记、`code/` 下 demo。书目：[全书目录-与实体书一致.md](../全书目录-与实体书一致.md)

| 书 § | 笔记 | Demo |
|------|------|------|
| 2.1 | [2.1-atomic-load-store.md](./2.1-atomic-load-store/2.1-atomic-load-store.md)（索引） | [2.1-atomic-load-store/code/](./2.1-atomic-load-store/code/) |
| 2.1.1 | [2.1.1-stop-flag.md](./2.1-atomic-load-store/2.1.1-stop-flag.md) | ↑ `code/` |
| 2.1.2 | [2.1.2-progress-reporting.md](./2.1-atomic-load-store/2.1.2-progress-reporting.md) | ↑ |
| 2.1.3 | [2.1.3-synchronization.md](./2.1-atomic-load-store/2.1.3-synchronization.md) | ↑ |
| 2.1.4 | [2.1.4-lazy-init.md](./2.1-atomic-load-store/2.1.4-lazy-init.md) | ↑ |
| 2.2 | [2.2-fetch-and-modify.md](./2.2-fetch-and-modify/2.2-fetch-and-modify.md)（索引） | [2.2-fetch-and-modify/code/](./2.2-fetch-and-modify/code/) |
| 2.2.1 | [2.2.1-multi-thread-progress.md](./2.2-fetch-and-modify/2.2.1-multi-thread-progress.md) | ↑ |
| 2.2.2 | [2.2.2-statistics.md](./2.2-fetch-and-modify/2.2.2-statistics.md) | ↑ |
| 2.2.3 | [2.2.3-id-allocation.md](./2.2-fetch-and-modify/2.2.3-id-allocation.md) | ↑ |
| 2.3 | [2.3-compare-and-exchange.md](./2.3-compare-and-exchange/2.3-compare-and-exchange.md)（索引） | [2.3-compare-and-exchange/code/](./2.3-compare-and-exchange/code/) |
| 2.3.1 | [2.3.1-id-no-overflow.md](./2.3-compare-and-exchange/2.3.1-id-no-overflow.md) | ↑ |
| 2.3.2 | [2.3.2-lazy-one-time-init.md](./2.3-compare-and-exchange/2.3.2-lazy-one-time-init.md) | ↑ |
| 2.4 | [2.4-summary.md](./2.4-summary/2.4-summary.md) | [2.4-summary/code/](./2.4-summary/code/) |

运行：`cargo build --manifest-path {MANIFEST}` · `study_atomic::chapter_02` 各 demo 模块""",
    )

    write(
        "mod.rs",
        """//! 第二章 — 书 §2.1～2.4 · 每节 `2.Y-slug/code/` demo

#[path = "2.1-atomic-load-store/code/2.1-atomic-load-store-demo.rs"]
pub mod use_atomic;

#[path = "2.1-atomic-load-store/code/2.1-atomic-load-store-lazy-init-demo.rs"]
pub mod lazy_init;

#[path = "2.2-fetch-and-modify/code/2.2-fetch-and-modify-demo.rs"]
pub mod use_atomic_operations;

#[path = "2.3-compare-and-exchange/code/2.3-compare-and-exchange-id-allocator-demo.rs"]
pub mod id_allocator;

#[path = "2.4-summary/code/2.4-summary-demo.rs"]
pub mod quick_demo;""",
    )

    print("Chapter-02 restructure: demos moved, index files next (run content writer)")


if __name__ == "__main__":
    main()
