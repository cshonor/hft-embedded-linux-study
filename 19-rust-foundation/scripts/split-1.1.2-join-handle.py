#!/usr/bin/env python3
"""Split 1.1.2-join-handle.md into 1.1.2.1–1.1.2.5 + index."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust"
SRC = DIR / "1.1.2-join-handle.md"

text = SRC.read_text(encoding="utf-8")
lines = text.splitlines(keepends=True)

# Line numbers are 1-based; slices use 0-based half-open indices.
chunks = {
    "1.1.2.1-join-handle-basics.md": (0, 58),  # intro + §一
    "1.1.2.2-spawn-join-types.md": (59, 139),  # §二 through join signature
    "1.1.2.3-panic-box-dyn-any.md": (140, 455),  # Err … strategy flow
    "1.1.2.4-panic-capture-downcast.md": (455, 756),  # 2.1 + 2.2
    "1.1.2.5-join-os-drop-pitfalls.md": (826, len(lines)),  # §三–§九 + demo
    "1.1.2.2-spawn-join-types-append": (756, 826),  # T vs Result block
}

# Merge T vs Result into 1.1.2.2
body_22 = "".join(lines[59:139]) + "".join(lines[756:826])

NAV = {
    "1.1.2.1-join-handle-basics.md": (
        "## 1.1.2.1 JoinHandle 是什么\n\n"
        "> 书 §1.1（2.1）· 索引：[1.1.2](./1.1.2-join-handle.md) · 前：[1.1.1 spawn/join](./1.1.1-spawn-join-basics.md) · 后：[1.1.2.2 spawn/join 类型](./1.1.2.2-spawn-join-types.md)\n\n---\n\n"
    ),
    "1.1.2.2-spawn-join-types.md": (
        "## 1.1.2.2 `spawn` / `join` 类型与 `T` vs `Result`\n\n"
        "> 书 §1.1（2.2）· 索引：[1.1.2](./1.1.2-join-handle.md) · 前：[1.1.2.1](./1.1.2.1-join-handle-basics.md) · 后：[1.1.2.3 panic 载荷](./1.1.2.3-panic-box-dyn-any.md) · FnOnce 语法：[1.1.5](./1.1.5-where-fnonce-syntax.md)\n\n---\n\n"
    ),
    "1.1.2.3-panic-box-dyn-any.md": (
        "## 1.1.2.3 `Box<dyn Any>`：panic 载荷与 `panic_any` / `anyhow`\n\n"
        "> 书 §1.1（2.3）· 索引：[1.1.2](./1.1.2-join-handle.md) · 前：[1.1.2.2](./1.1.2.2-spawn-join-types.md) · 后：[1.1.2.4 downcast 捕获](./1.1.2.4-panic-capture-downcast.md)\n\n---\n\n"
    ),
    "1.1.2.4-panic-capture-downcast.md": (
        "## 1.1.2.4 panic 捕获与 `downcast_ref`（含 turbofish）\n\n"
        "> 书 §1.1（2.4）· 索引：[1.1.2](./1.1.2-join-handle.md) · 前：[1.1.2.3](./1.1.2.3-panic-box-dyn-any.md) · 后：[1.1.2.5 OS / drop](./1.1.2.5-join-os-drop-pitfalls.md)\n\n---\n\n"
    ),
    "1.1.2.5-join-os-drop-pitfalls.md": (
        "## 1.1.2.5 易错点、OS 底层与 drop detach\n\n"
        "> 书 §1.1（2.5）· 索引：[1.1.2](./1.1.2-join-handle.md) · 前：[1.1.2.4 downcast](./1.1.2.4-panic-capture-downcast.md) · 分离态：[1.1.7](./1.1.7-thread-lifecycle-four-states.md) · 后：[1.1.3 Send/'static](./1.1.3-send-static-spawn.md)\n\n---\n\n"
    ),
}

# Strip old top heading from chunk bodies (first ## 1.1.2 line through first ---)
def strip_old_header(chunk: str) -> str:
    if chunk.startswith("## 1.1.2 JoinHandle"):
        parts = chunk.split("---\n", 2)
        if len(parts) >= 3:
            return parts[2].lstrip("\n")
        parts = chunk.split("---\n", 1)
        if len(parts) >= 2:
            return parts[1].lstrip("\n")
    return chunk


def fix_links(s: str, fname: str) -> str:
    repl = {
        "[§二 Err 三层拆解](#二err-类型三层拆解-boxdyn-any--send--static)": "[1.1.2.3 Err 三层](./1.1.2.3-panic-box-dyn-any.md#err-类型三层拆解boxdyn-any--send--static)",
        "[§四](#四不用-join-会怎样)": "[1.1.2.5 §四](./1.1.2.5-join-os-drop-pitfalls.md#四不用-join-会怎样)",
        "[§2.1](#21-panic-捕获与-downcast_ref)": "[1.1.2.4 §2.1](./1.1.2.4-panic-capture-downcast.md#21-panic-捕获与-downcast_ref)",
        "[§2.2](#22-downcast_refstr-类型与层级精读)": "[1.1.2.4 §2.2](./1.1.2.4-panic-capture-downcast.md#22-downcast_refstrturbofish-与类型层级精读)",
        "[§2.2](#22-downcast_refstr-类型与层级精读)": "[1.1.2.4 §2.2](./1.1.2.4-panic-capture-downcast.md#22-downcast_refstrturbofish-与类型层级精读)",
        "[§与 anyhow](#与-anyhow把-join-的-err-并进-result---主流程)": "[1.1.2.3 §anyhow](./1.1.2.3-panic-box-dyn-any.md#与-anyhow把-join-的-err-并进-result---主流程)",
        "[§Err 设计动机](#设计动机子线程-panic-载荷完全不可控)": "[1.1.2.3 §设计动机](./1.1.2.3-panic-box-dyn-any.md#设计动机子线程-panic-载荷完全不可控)",
        "[§主动 vs 自动 panic](#主动-panic-vs-运行时自动-panic来源不同汇合点相同)": "[1.1.2.3 §主动 vs 自动](./1.1.2.3-panic-box-dyn-any.md#主动-panic-vs-运行时自动-panic来源不同汇合点相同)",
        "[§为何先 &str 再 String](#为何-downcast-顺序先-str-再-string)": "[1.1.2.4 §downcast 顺序](./1.1.2.4-panic-capture-downcast.md#为何-downcast-顺序先-str-再-string)",
        "[§panic 与 match 分工](#panic-与-match-的分工抛-vs-接)": "[1.1.2.3 §panic 与 match](./1.1.2.3-panic-box-dyn-any.md#panic-与-match-的分工抛-vs-接)",
        "[§panic_any vs panic!](#panic-与-panic_any互补不是替代)": "[1.1.2.3 §panic_any](./1.1.2.3-panic-box-dyn-any.md#panic-与-panic_any互补不是替代)",
        "[§二 `T` vs `Result`](#joinhandlet-的-t-vs-join-的-result两个独立层次)": "[1.1.2.2 §T vs Result](./1.1.2.2-spawn-join-types.md#joinhandlet-的-t-vs-join-的-result两个独立层次)",
        "[§八 drop 而不 join](#八drop-而不-join)": "[1.1.2.5 §八](./1.1.2.5-join-os-drop-pitfalls.md#八drop-而不-join)",
    }
    for old, new in repl.items():
        s = s.replace(old, new)
    s = s.replace("§1.1 索引：[1.1-threads-in-rust.md](./1.1-threads-in-rust.md)", "§1.1.2 索引：[1.1.2-join-handle.md](./1.1.2-join-handle.md) · §1.1：[1.1-threads-in-rust.md](./1.1-threads-in-rust.md)")
    return s


files = {
    "1.1.2.1-join-handle-basics.md": strip_old_header("".join(lines[0:58])),
    "1.1.2.2-spawn-join-types.md": strip_old_header(body_22),
    "1.1.2.3-panic-box-dyn-any.md": strip_old_header("".join(lines[140:455])),
    "1.1.2.4-panic-capture-downcast.md": strip_old_header("".join(lines[455:756])),
    "1.1.2.5-join-os-drop-pitfalls.md": strip_old_header("".join(lines[826:])),
}

for fname, body in files.items():
    out = NAV[fname] + fix_links(body, fname)
    (DIR / fname).write_text(out, encoding="utf-8", newline="\n")
    print("wrote", fname, len(out))

INDEX = """## 1.1.2 JoinHandle\\<T\\>（索引）

> 书 §1.1（中）· 入门：[1.1.1 spawn/join](./1.1.1-spawn-join-basics.md) · **四种状态：[1.1.7](./1.1.7-thread-lifecycle-four-states.md)** · 后续：[1.1.3 Send/'static](./1.1.3-send-static-spawn.md)

**本节正文已拆分**；按 **1.1.2.1 → 1.1.2.5** 顺序阅读。

---

## §1.1.2 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **1.1.2.1** | JoinHandle 是什么、返回值、`join` 入门 | [1.1.2.1-join-handle-basics.md](./1.1.2.1-join-handle-basics.md) |
| **1.1.2.2** | `spawn`/`join` 签名、`T=()`、`T` vs `Result` | [1.1.2.2-spawn-join-types.md](./1.1.2.2-spawn-join-types.md) |
| **1.1.2.3** | `Box<dyn Any>`、`panic_any`、panic/match 分工、`anyhow` | [1.1.2.3-panic-box-dyn-any.md](./1.1.2.3-panic-box-dyn-any.md) |
| **1.1.2.4** | `downcast_ref`、turbofish、`&&str`、捕获顺序 | [1.1.2.4-panic-capture-downcast.md](./1.1.2.4-panic-capture-downcast.md) |
| **1.1.2.5** | 易错点、OS `pthread_join`、drop detach | [1.1.2.5-join-os-drop-pitfalls.md](./1.1.2.5-join-os-drop-pitfalls.md) |

**阅读顺序**：**1.1.2.1 → 1.1.2.2 → 1.1.2.3 → 1.1.2.4 → 1.1.2.5** → [1.1.7 四种状态](./1.1.7-thread-lifecycle-four-states.md)（可与 1.1.2 交叉对照）

---

## Demo 一览

| 主题 | 文件 |
|------|------|
| 子线程返回值 | [return-demo.rs](./code/1.1-threads-in-rust-return-demo.rs) |
| **`join` + panic / `downcast_ref`** | [join-panic-demo.rs](./code/1.1-threads-in-rust-join-panic-demo.rs) |

§1.1 索引：[1.1-threads-in-rust.md](./1.1-threads-in-rust.md)
"""

(DIR / "1.1.2-join-handle.md").write_text(INDEX.replace("\\<", "<").replace("\\>", ">"), encoding="utf-8", newline="\n")
print("wrote index")
