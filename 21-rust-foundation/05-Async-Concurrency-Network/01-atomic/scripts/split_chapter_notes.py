# -*- coding: utf-8 -*-
"""Split 本章学习笔记.md into per-section files with migrated content."""
from __future__ import annotations

from pathlib import Path

ATOMIC = Path(__file__).resolve().parent.parent

PATH_REPLACEMENTS = [
    ("2.2-load-store/", "2.1-atomic-load-store/"),
    ("2.3-fetch-modify/", "2.2-fetch-and-modify/"),
    ("2.4-cas/", "2.3-compare-and-exchange/"),
    ("2.5-quick-demo/", "2.4-summary/"),
    ("2.6-fence/", "Chapter-03-Memory-Ordering/3.4-fences/"),
    ("2.7-seqcst/", "Chapter-03-Memory-Ordering/3.3-memory-order-options/"),
    ("4.1-spin-lock/", "4.1-minimal-implementation/"),
    ("5.1-one-shot-channel/", "5.2-unsafe-one-shot-channel/"),
    ("6.1-custom-arc/", "6.1-basic-reference-counting/"),
    ("6.1-custom-arc-demo.rs", "6.1-basic-reference-counting-demo.rs"),
]


def fix_paths(text: str) -> str:
    for old, new in PATH_REPLACEMENTS:
        text = text.replace(old, new)
    return text


def split_by_h2(src: str) -> dict[str, str]:
    parts: dict[str, str] = {}
    current_key = "_preamble"
    current_lines: list[str] = []
    for line in src.splitlines(keepends=True):
        if line.startswith("## ") and not line.startswith("### "):
            parts[current_key] = "".join(current_lines)
            current_key = line[3:].strip()
            current_lines = [line]
        else:
            current_lines.append(line)
    parts[current_key] = "".join(current_lines)
    return parts


def section_slice(text: str, start: str, end: str | None = None) -> str:
    if start not in text:
        return ""
    i = text.index(start)
    chunk = text[i:]
    if end:
        rest = chunk[len(start) :]
        if end in rest:
            chunk = chunk[: len(start) + rest.index(end)]
    return chunk


def write_section(ch: Path, fname: str, chunks: tuple[str, ...], book_title: str) -> None:
    body = fix_paths("".join(chunks)).strip()
    if not body:
        return
    if not body.startswith("#"):
        body = f"# {book_title}\n\n{body}\n"
    footer = (
        "\n---\n\n"
        f"> 章节索引：[本章学习笔记.md](./本章学习笔记.md) · "
        "[全书目录](../全书目录-与实体书一致.md)\n"
    )
    (ch / fname).write_text(body + footer, encoding="utf-8")
    print("Wrote", ch.name, fname)


def write_index(ch: Path, title: str, rows: list[tuple[str, str, str]]) -> None:
    lines = [
        f"# {title}\n",
        "> **正文已拆至各小节文件**（非占位）。书目：[全书目录-与实体书一致.md](../全书目录-与实体书一致.md)\n",
        "| 书 § | 笔记 | Demo |",
        "|------|------|------|",
    ]
    for sec, note, demo in rows:
        demo_cell = f"[{demo}/](./{demo}/)" if demo else "—"
        lines.append(f"| {sec} | [{note}](./{note}) | {demo_cell} |")
    lines.append("")
    (ch / "本章学习笔记.md").write_text("\n".join(lines), encoding="utf-8")
    print("Wrote index", ch.name)


def split_ch01():
    ch = ATOMIC / "Chapter-01-Rust-Concurrency-Basics"
    src = (ch / "本章学习笔记.md").read_text(encoding="utf-8")
    if "正文已拆至各小节" in src:
        print("Ch01 already split, skip")
        return
    # restore from git if index only
    print("Ch01: run split only when full 本章学习笔记 present; use prior split_ch01 in repo")


def split_ch02():
    ch = ATOMIC / "Chapter-02-Atomics"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    preamble = p.get("_preamble", "")
    s2 = p.get("2. 第二章精读：原子操作基础", "")
    mapping = {
        "2.1-atomic-load-store.md": (
            preamble,
            p.get("0. 三章分工（避免「只读第二章却缺理论）", ""),
            p.get("1. 内存模型、可见性与数据竞争底层", ""),
            section_slice(s2, "## 2. 第二章精读", "### 2.1"),
            section_slice(s2, "### 2.1", "### 2.2"),
        ),
        "2.2-fetch-and-modify.md": (section_slice(s2, "### 2.2", "### 2.3"),),
        "2.3-compare-and-exchange.md": (section_slice(s2, "### 2.3", "### 2.4"),),
        "2.4-summary.md": (
            section_slice(s2, "### 2.4", None),
            p.get("3. 日常开发规避数据竞争（实操法则）", ""),
            p.get("4. 核心术语（本章 + 跨章）", ""),
            p.get("5. 面试 / 笔试一句话", ""),
            p.get("6. 极简背诵卡（约 30 字 / 条）", ""),
            p.get("7. 与 `llvm_insight` 的衔接（可选）", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第二章 — 学习笔记索引（实体书 §2.1～2.4）",
        [
            ("2.1", "2.1-atomic-load-store.md", "2.1-atomic-load-store"),
            ("2.2", "2.2-fetch-and-modify.md", "2.2-fetch-and-modify"),
            ("2.3", "2.3-compare-and-exchange.md", "2.3-compare-and-exchange"),
            ("2.4", "2.4-summary.md", "2.4-summary"),
        ],
    )


def split_ch03():
    ch = ATOMIC / "Chapter-03-Memory-Ordering"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    s2 = p.get("2. 五大内存顺序（精读）", "")
    mapping = {
        "3.1-reordering-and-optimizations.md": (p.get("_preamble", ""), p.get("1. 为什么需要内存顺序？", "")),
        "3.2-the-memory-model.md": (
            "## 3.2 The Memory Model\n\n",
            section_slice(s2, "## 2. 五大", "### ①"),
            section_slice(s2, "### ①", "### ②"),
        ),
        "3.3-memory-order-options.md": (
            section_slice(s2, "### ②", "### `fence`"),
            section_slice(s2, "### ③", "### ④"),
            section_slice(s2, "### ④", "### ⑤"),
            section_slice(s2, "### ⑤", "### 对照表"),
            section_slice(s2, "### 对照表", None),
        ),
        "3.4-fences.md": (section_slice(s2, "### `fence`", "### 对照表"),),
        "3.5-common-misconceptions.md": (p.get("4. 新手易错点与核心认知", ""),),
        "3.6-summary.md": (
            p.get("3. 选型原则（从弱到强）", ""),
            p.get("5. 配套代码", ""),
            p.get("6. 面试一句话", ""),
            p.get("7. 极简背诵卡", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第三章 — 学习笔记索引（§3.1～3.6）",
        [
            ("3.1", "3.1-reordering-and-optimizations.md", ""),
            ("3.2", "3.2-the-memory-model.md", ""),
            ("3.3", "3.3-memory-order-options.md", "3.3-memory-order-options"),
            ("3.4", "3.4-fences.md", "3.4-fences"),
            ("3.5", "3.5-common-misconceptions.md", ""),
            ("3.6", "3.6-summary.md", ""),
        ],
    )


def split_ch04():
    ch = ATOMIC / "Chapter-04-Spin-Locks"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    s2 = p.get("2. 核心结构与内存序匹配", "")
    mapping = {
        "4.1-minimal-implementation.md": (
            p.get("_preamble", ""),
            p.get("1. 自旋锁是什么", ""),
            s2.split("### 为何用 `UnsafeCell`")[0],
        ),
        "4.2-unsafe-spin-lock.md": (
            "## 4.2 Unsafe Spin Lock\n\n",
            section_slice(s2, "### 为何用 `UnsafeCell`", None),
        ),
        "4.3-safe-lock-guard.md": (p.get("3. 运行逻辑（对照 `demo()`）", ""),),
        "4.4-summary.md": (
            p.get("4. 选型与易错点（第 4 章视角）", ""),
            p.get("5. 面试一句话", ""),
            p.get("6. 极简背诵卡", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第四章 — 学习笔记索引（§4.1～4.4）",
        [
            ("4.1", "4.1-minimal-implementation.md", "4.1-minimal-implementation"),
            ("4.2", "4.2-unsafe-spin-lock.md", "4.1-minimal-implementation"),
            ("4.3", "4.3-safe-lock-guard.md", "4.1-minimal-implementation"),
            ("4.4", "4.4-summary.md", ""),
        ],
    )


def split_ch05():
    ch = ATOMIC / "Chapter-05-Channels"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    s2 = p.get("2. 核心设计（四个问题）", "")
    mapping = {
        "5.1-mutex-based-channel.md": (
            p.get("_preamble", ""),
            p.get("1. 本章在全书中的位置", ""),
            "## 5.1 Mutex-Based Channel\n\n",
            "基于 `Mutex` + `Condvar` + `VecDeque` 的多生产者/消费者教学通道，见 "
            "`5.1-mutex-based-channel/5.1-mutex-based-channel-demo.rs`。\n",
        ),
        "5.2-unsafe-one-shot-channel.md": (
            section_slice(s2, "### 2.1", "### 2.3"),
            section_slice(s2, "### 2.2", "### 2.3"),
            section_slice(s2, "### 2.3", "### 2.4"),
            section_slice(s2, "### 2.4", None),
            p.get("3. 与书中精简代码的对照", ""),
        ),
        "5.3-runtime-checks-safety.md": (
            "## 5.3 Safety Through Runtime Checks\n\n",
            section_slice(s2, "### 2.2", "### 2.3"),
            "运行时可用 `AtomicBool` + `Acquire` 检查就绪；编译期仍推荐类型消耗 `self`。\n",
        ),
        "5.4-types-safety.md": (section_slice(s2, "### 2.3", "### 2.4"),),
        "5.5-borrowing-avoid-allocation.md": (section_slice(s2, "### 2.1", "### 2.2"),),
        "5.6-blocking.md": (section_slice(s2, "### 2.4", None),),
        "5.7-summary.md": (
            p.get("4. 业务落地场景", ""),
            p.get("5. 面试一句话", ""),
            p.get("6. 极简背诵卡", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第五章 — 学习笔记索引（§5.1～5.7）",
        [
            ("5.1", "5.1-mutex-based-channel.md", "5.1-mutex-based-channel"),
            ("5.2", "5.2-unsafe-one-shot-channel.md", "5.2-unsafe-one-shot-channel"),
            ("5.3", "5.3-runtime-checks-safety.md", ""),
            ("5.4", "5.4-types-safety.md", ""),
            ("5.5", "5.5-borrowing-avoid-allocation.md", ""),
            ("5.6", "5.6-blocking.md", ""),
            ("5.7", "5.7-summary.md", ""),
        ],
    )


def split_ch06():
    ch = ATOMIC / "Chapter-06-Custom-Arc"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    mapping = {
        "6.1-basic-reference-counting.md": (
            p.get("_preamble", ""),
            p.get("1. 为什么手写 Arc？", ""),
            p.get("2. 控制块与指针", ""),
            p.get("3. `new` 与 `clone`", ""),
            p.get("4. `Drop` 与最后释放", ""),
        ),
        "6.2-testing-it.md": (
            "## 6.2 Testing It\n\n",
            p.get("7. 示例与运行", ""),
            "见 `6.2-testing-it/6.2-testing-it-demo.rs`（标准库 `Arc` 计数 smoke test）。\n",
        ),
        "6.3-mutation.md": (
            "## 6.3 Mutation\n\n",
            "单所有者 `Arc::get_mut`；多所有者用 `Arc<Mutex<T>>`。见 `6.3-mutation/6.3-mutation-demo.rs`。\n",
        ),
        "6.4-weak-pointers.md": (
            "## 6.4 Weak Pointers\n\n",
            "独立 `weak_count`，`upgrade` 用 CAS 尝试增加 `strong`。见 `6.4-weak-pointers/6.4-weak-pointers-demo.rs`。\n",
        ),
        "6.5-testing-weak.md": (
            "## 6.5 Testing Weak\n\n",
            "在 `6.4` demo 中 `drop(arc)` 后 `upgrade` 失败路径即弱引用测试。\n",
        ),
        "6.6-optimizing.md": (
            "## 6.6 Optimizing\n\n",
            "计数与热数据 cache line 分离：`6.6-optimizing/6.6-optimizing-align-demo.rs`。\n",
        ),
        "6.7-summary.md": (
            p.get("5. `Send` / `Sync`", ""),
            p.get("6. 与标准库差异（本书未全实现部分）", ""),
            p.get("8. 小结", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第六章 — 学习笔记索引（§6.1～6.7）",
        [
            ("6.1", "6.1-basic-reference-counting.md", "6.1-basic-reference-counting"),
            ("6.2", "6.2-testing-it.md", "6.2-testing-it"),
            ("6.3", "6.3-mutation.md", "6.3-mutation"),
            ("6.4", "6.4-weak-pointers.md", "6.4-weak-pointers"),
            ("6.5", "6.5-testing-weak.md", ""),
            ("6.6", "6.6-optimizing.md", "6.6-optimizing"),
            ("6.7", "6.7-summary.md", ""),
        ],
    )


def split_ch07():
    ch = ATOMIC / "Chapter-07-Processors"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    mapping = {
        "7.1-processor-instructions.md": (p.get("_preamble", ""), p.get("1. 处理器指令（§7.1）", "")),
        "7.2-caching.md": (p.get("2. 缓存（§7.2）", ""),),
        "7.3-reordering.md": (p.get("3. 重排（§7.3）", ""),),
        "7.4-memory-fences.md": (p.get("4. 内存屏障（§7.4）", ""),),
        "7.5-summary.md": (p.get("5. 小结（§7.5）", ""),),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第七章 — 学习笔记索引（§7.1～7.5）",
        [
            ("7.1", "7.1-processor-instructions.md", ""),
            ("7.2", "7.2-caching.md", "7.2-caching"),
            ("7.3", "7.3-reordering.md", "7.3-reordering"),
            ("7.4", "7.4-memory-fences.md", ""),
            ("7.5", "7.5-summary.md", ""),
        ],
    )


def split_ch10():
    ch = ATOMIC / "Chapter-10-Advanced-Concurrent-Data-Structures"
    src = fix_paths((ch / "本章学习笔记.md").read_text(encoding="utf-8"))
    p = split_by_h2(src)
    s1 = p.get("1. 第十章核心：进阶并发设计模式", "")
    s2 = p.get("2. 无锁编程：目标、基石与选型", "")
    mapping = {
        "10.1-semaphores.md": (section_slice(s1, "### 1.1", "### 1.2"),),
        "10.2-rcu.md": (section_slice(s1, "### 1.2", "### 1.3"), section_slice(s2, "### 2.2", "### 2.3")),
        "10.3-lock-free-linked-list.md": (
            section_slice(s1, "### 1.3", "### 1.4"),
            section_slice(s2, "### 2.3", "### 2.4"),
        ),
        "10.4-queue-based-locks.md": (section_slice(s1, "### 1.4", "### 1.5"),),
        "10.5-blocking-locks.md": (p.get("3. 高并发 Rust 程序整体设计思路（全书自顶向下）", ""),),
        "10.6-seqlocks.md": (section_slice(s1, "### 1.5", None),),
        "10.7-teaching-materials.md": (
            p.get("4. 性能优化方向与最佳实践", ""),
            p.get("5. 调试排错与并发陷阱", ""),
            p.get("6. 面试核心考点（全书浓缩）", ""),
            p.get("7. 与仓库其他模块", ""),
            p.get("8. 背诵卡", ""),
        ),
    }
    for fname, chunks in mapping.items():
        write_section(ch, fname, chunks, fname.replace(".md", ""))
    write_index(
        ch,
        "第十章 — 学习笔记索引（§10.1～10.7）",
        [
            ("10.1", "10.1-semaphores.md", "10.1-semaphores"),
            ("10.2", "10.2-rcu.md", "10.2-rcu"),
            ("10.3", "10.3-lock-free-linked-list.md", "10.3-lock-free-linked-list"),
            ("10.4", "10.4-queue-based-locks.md", ""),
            ("10.5", "10.5-blocking-locks.md", ""),
            ("10.6", "10.6-seqlocks.md", ""),
            ("10.7", "10.7-teaching-materials.md", ""),
        ],
    )


def main():
    split_ch02()
    split_ch03()
    split_ch04()
    split_ch05()
    split_ch06()
    split_ch07()
    split_ch10()
    print("Done.")


if __name__ == "__main__":
    main()
