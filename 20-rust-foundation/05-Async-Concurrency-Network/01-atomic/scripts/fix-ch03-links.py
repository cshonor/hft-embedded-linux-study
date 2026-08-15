#!/usr/bin/env python3
"""Fix stale relative links after Ch03 restructure."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "Chapter-03-Memory-Ordering"

REPLACEMENTS = [
    # 3.3 index → chapter index
    (
        ROOT / "3.3-memory-order-options" / "3.3-memory-order-options.md",
        "[本章学习笔记.md](./本章学习笔记.md)",
        "[本章学习笔记.md](../本章学习笔记.md)",
    ),
    # 3.3.1 → 3.1
    (
        ROOT / "3.3-memory-order-options" / "3.3.1-relaxed.md",
        "[3.1 反例](./3.1-reordering-and-optimizations.md)",
        "[3.1 反例](../3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md)",
    ),
    # 3.3.2 → 3.1
    (
        ROOT / "3.3-memory-order-options" / "3.3.2-release-acquire.md",
        "[3.1](./3.1-reordering-and-optimizations.md)",
        "[3.1](../3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md)",
    ),
    # 3.3.5 → 3.5 / 3.6
    (
        ROOT / "3.3-memory-order-options" / "3.3.5-ordering-compare-select.md",
        "[3.6 总结](./3.6-summary.md)",
        "[3.6 总结](../3.6-summary/3.6-summary.md)",
    ),
    (
        ROOT / "3.3-memory-order-options" / "3.3.5-ordering-compare-select.md",
        "[3.5 误区](./3.5-common-misconceptions.md)",
        "[3.5 误区](../3.5-common-misconceptions/3.5-common-misconceptions.md)",
    ),
    # 3.4 fences
    (
        ROOT / "3.4-fences" / "3.4-fences.md",
        "[本章学习笔记.md](./本章学习笔记.md)",
        "[本章学习笔记.md](../本章学习笔记.md)",
    ),
    (
        ROOT / "3.4-fences" / "3.4-fences.md",
        "| [use_fence.rs]((已移除；见 mod.rs use_fence)) | 贯通笔记配套 |",
        "| `use_fence` 模块 | 见章根 `mod.rs` |",
    ),
    # 3.5
    (
        ROOT / "3.5-common-misconceptions" / "3.5-common-misconceptions.md",
        "前：[3.4 Fences](./3.4-fences.md)",
        "前：[3.4 Fences](../3.4-fences/3.4-fences.md)",
    ),
    (
        ROOT / "3.5-common-misconceptions" / "3.5-common-misconceptions.md",
        "[本章学习笔记.md](./本章学习笔记.md)",
        "[本章学习笔记.md](../本章学习笔记.md)",
    ),
]

# 3.6 summary — multiple fixes
p36 = ROOT / "3.6-summary" / "3.6-summary.md"
text = p36.read_text(encoding="utf-8")
text = text.replace(
    "[2.1-atomic-load-store-demo.rs](../Chapter-02-Atomics/2.1-atomic-load-store/2.1-atomic-load-store-demo.rs)",
    "[2.1-atomic-load-store-demo.rs](../Chapter-02-Atomics/2.1-atomic-load-store/code/2.1-atomic-load-store-demo.rs)",
)
text = text.replace(
    "[3.4-fences-demo.rs](./code/3.4-fences-demo.rs)",
    "[3.4-fences-demo.rs](../3.4-fences/code/3.4-fences-demo.rs)",
)
text = text.replace(
    "cargo build --manifest-path atomic/Cargo.toml",
    "cargo build --manifest-path 05-Async-Concurrency-Network/01-atomic/Cargo.toml",
)
text = text.replace(
    "[3.3.5 对照表](./3.3.5-ordering-compare-select.md#对照表必背)",
    "[3.3.5 对照表](../3.3-memory-order-options/3.3.5-ordering-compare-select.md#对照表必背)",
)
text = text.replace(
    "[本章学习笔记.md](./本章学习笔记.md)",
    "[本章学习笔记.md](../本章学习笔记.md)",
)
p36.write_text(text, encoding="utf-8")
print("patched", p36.relative_to(ROOT.parent))

for path, old, new in REPLACEMENTS:
    t = path.read_text(encoding="utf-8")
    if old not in t:
        raise SystemExit(f"missing pattern in {path}: {old!r}")
    path.write_text(t.replace(old, new), encoding="utf-8")
    print("patched", path.relative_to(ROOT.parent))

print("ch03 link fixes done")
