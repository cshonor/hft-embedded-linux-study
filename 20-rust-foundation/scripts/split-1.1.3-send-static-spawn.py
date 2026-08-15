#!/usr/bin/env python3
"""Split 1.1.3-send-static-spawn.md into 1.1.3.1–1.1.3.6 sub-notes + index."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIR = ROOT / "05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust"
SRC = DIR / "1.1.3-send-static-spawn.md"

# line numbers 1-based inclusive
SLICES = [
    (
        "1.1.3.1-spawn-signature-constraints.md",
        "## 1.1.3.1 `spawn` 签名与四条约束",
        "> 书 §1.1（3.1）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.2 JoinHandle](./1.1.2-join-handle.md) · 后：[1.1.3.2 Send](./1.1.3.2-send-why.md)",
        13,
        164,
    ),
    (
        "1.1.3.2-send-why.md",
        "## 1.1.3.2 为什么必须 `Send`",
        "> 书 §1.1（3.2）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.3.1 签名](./1.1.3.1-spawn-signature-constraints.md) · 后：[1.1.3.3 `'static` 设计](./1.1.3.3-static-why-design.md)",
        166,
        180,
    ),
    (
        "1.1.3.3-static-why-design.md",
        "## 1.1.3.3 为什么必须 `'static`：场景、调度、独立线程",
        "> 书 §1.1（3.3）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.3.2 Send](./1.1.3.2-send-why.md) · 后：[1.1.3.4 `T: 'static`](./1.1.3.4-t-static-types.md)",
        182,
        325,
    ),
    (
        "1.1.3.4-t-static-types.md",
        "## 1.1.3.4 `T: 'static` 语义：`String` / 借引用 / `Arc`",
        "> 书 §1.1（3.4）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.3.3 `'static` 设计](./1.1.3.3-static-why-design.md) · 后：[1.1.3.5 move / 编译错误](./1.1.3.5-move-capture-errors.md)",
        327,
        395,
    ),
    (
        "1.1.3.5-move-capture-errors.md",
        "## 1.1.3.5 `move`、最小化捕获与编译器报错",
        "> 书 §1.1（3.5）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.3.4 `T: 'static`](./1.1.3.4-t-static-types.md) · 后：[1.1.3.6 Arc / scope / HFT](./1.1.3.6-arc-scope-hft-cheatsheet.md)",
        397,
        520,
    ),
    (
        "1.1.3.6-arc-scope-hft-cheatsheet.md",
        "## 1.1.3.6 `Arc`、`thread::scope`、HFT 与对照表",
        "> 书 §1.1（3.6）· 索引：[1.1.3](./1.1.3-send-static-spawn.md) · 前：[1.1.3.5 move / 编译错误](./1.1.3.5-move-capture-errors.md) · 后：[1.1.4 FnOnce](./1.1.4-closure-traits-fnonce.md) · scope：[1.2](../1.2-scoped-threads/1.2-scoped-threads.md)",
        522,
        640,
    ),
]

LINK_REPLACEMENTS = [
    ("[§三](#三为什么必须-static)", "[1.1.3.3 `'static` 设计](./1.1.3.3-static-why-design.md)"),
    ("[§3.4](#34-tstatic-到底是什么)", "[1.1.3.4 `T: 'static`](./1.1.3.4-t-static-types.md#tstatic-到底是什么)"),
    ("[§3.7](#37-spawn-两种传参借用-vs-move)", "[1.1.3.5 move / 捕获](./1.1.3.5-move-capture-errors.md#spawn-两种传参借用-vs-move)"),
    ("[§五](#五正确写法move-转移所有权)", "[1.1.3.5 §正确写法 move](./1.1.3.5-move-capture-errors.md#五正确写法move-转移所有权)"),
    ("[§七](#七绕过-staticthreadscope-为什么可以借局部变量)", "[1.1.3.6 scope](./1.1.3.6-arc-scope-hft-cheatsheet.md#七绕过-staticthreadscope-为什么可以借局部变量)"),
    ("[§八](#八hft-语境与学习顺序)", "[1.1.3.6 HFT](./1.1.3.6-arc-scope-hft-cheatsheet.md#八hft-语境与学习顺序)"),
    ("（[§3.4](#34-tstatic-到底是什么)）", "（[1.1.3.4](./1.1.3.4-t-static-types.md#tstatic-到底是什么)）"),
    ("见 [§3.4](#34-tstatic-到底是什么)", "见 [1.1.3.4](./1.1.3.4-t-static-types.md#tstatic-到底是什么)"),
    ("详见 [§三](#三为什么必须-static)", "详见 [1.1.3.3](./1.1.3.3-static-why-design.md)"),
    ("（见 [§三](#三为什么必须-static)）", "（见 [1.1.3.3](./1.1.3.3-static-why-design.md)）"),
]


def fix_links(text: str) -> str:
    for old, new in LINK_REPLACEMENTS:
        text = text.replace(old, new)
    return text


def main() -> None:
    lines = SRC.read_text(encoding="utf-8").splitlines(keepends=True)

    for fname, title, nav, start, end in SLICES:
        body = "".join(lines[start - 1 : end])
        body = fix_links(body)
        # demote ## section headers to ### (keep ### as #### for 3.x subsections)
        out_lines = []
        for line in body.splitlines(keepends=True):
            if line.startswith("## ") and not line.startswith("###"):
                out_lines.append("#" + line)  # ## -> ###
            elif line.startswith("### ") and not line.startswith("####"):
                out_lines.append("#" + line)  # ### -> ####
            else:
                out_lines.append(line)
        body = "".join(out_lines)
        content = f"{title}\n\n{nav}\n\n---\n\n{body}\n"
        (DIR / fname).write_text(content, encoding="utf-8")
        print(f"wrote {fname}")

    index = """## 1.1.3 spawn 的 `Send + 'static` 约束（索引）

> 书 §1.1（续）· [1.1.1 spawn/join](./1.1.1-spawn-join-basics.md) · [1.1.2 JoinHandle](./1.1.2-join-handle.md) · 绕过 `'static`：[1.2 scope](../1.2-scoped-threads/1.2-scoped-threads.md) · [1.3 Arc](../1.3-shared-ownership/1.3-shared-ownership.md) · [1.6 Send/Sync](../1.6-send-sync/1.6-send-sync.md)

**本节正文已拆分**；按 **1.1.3.1 → 1.1.3.6** 顺序阅读。

**结论速览**：

1. **`T: 'static` 不只修饰引用**——自有类型、`Arc`、结构体等都可以满足；含义是类型内部**不含任何短于 `'static` 的借用**（[1.1.3.4](./1.1.3.4-t-static-types.md)）。
2. **`spawn` 要防的是场景 B**：创建线程的**函数先 return**、局部栈释放，子线程仍可能被调度（不是「`main` 跑完子线程还在跑」）（[1.1.3.3](./1.1.3.3-static-why-design.md)）。
3. **`move` 自有类型进闭包** → 闭包内无短命引用 → **`F: 'static` 天然成立**（[1.1.3.5](./1.1.3.5-move-capture-errors.md)）；借栈 → **`thread::scope`**（[1.1.3.6](./1.1.3.6-arc-scope-hft-cheatsheet.md)）。

---

## §1.1.3 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **1.1.3.1** | `spawn` 签名、四条约束、`F`/`T` 双重 `'static` | [1.1.3.1-spawn-signature-constraints.md](./1.1.3.1-spawn-signature-constraints.md) |
| **1.1.3.2** | 为什么必须 **`Send`** | [1.1.3.2-send-why.md](./1.1.3.2-send-why.md) |
| **1.1.3.3** | 为什么 **`'static`**：场景 A/B、调度 vs 栈回收、独立线程 vs scope | [1.1.3.3-static-why-design.md](./1.1.3.3-static-why-design.md) |
| **1.1.3.4** | **`T: 'static` 语义**、`&'static T` 区别、`String`/`Arc` | [1.1.3.4-t-static-types.md](./1.1.3.4-t-static-types.md) |
| **1.1.3.5** | **`move`**、最小化捕获、**E0373** 编译失败、正确写法 | [1.1.3.5-move-capture-errors.md](./1.1.3.5-move-capture-errors.md) |
| **1.1.3.6** | **`Arc`**、**`thread::scope`**、HFT 选型、对照表、背诵版 | [1.1.3.6-arc-scope-hft-cheatsheet.md](./1.1.3.6-arc-scope-hft-cheatsheet.md) |

**阅读顺序**：**1.1.3.1 → 1.1.3.2 → 1.1.3.3 → 1.1.3.4 → 1.1.3.5 → 1.1.3.6** → [1.1.4 FnOnce](./1.1.4-closure-traits-fnonce.md)

---

## Demo 一览

| 主题 | 文件 |
|------|------|
| `move` 闭包 | [move-closure-demo.rs](./code/1.1-threads-in-rust-move-closure-demo.rs) |
| **`spawn` 无 move 编译失败** | [spawn-no-move-compile-fail.rs](./code/1.1-threads-in-rust-spawn-no-move-compile-fail.rs) |
| **`spawn` `'static` vs `scope` 借栈** | [static-vs-scope-demo.rs](./code/1.1-threads-in-rust-static-vs-scope-demo.rs) |
| `Arc` 跨线程（章内） | [1.3-shared-ownership-arc-demo.rs](../1.3-shared-ownership/code/1.3-shared-ownership-arc-demo.rs) |
| `scope` 借局部 | [1.2-scoped-threads-demo.rs](../1.2-scoped-threads/code/1.2-scoped-threads-demo.rs) |

§1.1 索引：[1.1-threads-in-rust.md](./1.1-threads-in-rust.md)
"""
    SRC.write_text(index, encoding="utf-8")
    print("wrote index 1.1.3-send-static-spawn.md")


if __name__ == "__main__":
    main()
