#!/usr/bin/env python3
from pathlib import Path

CH3 = Path(__file__).resolve().parents[1] / "Chapter-03-Memory-Ordering"
M = "05-Async-Concurrency-Network/01-atomic/Cargo.toml"


def w(rel: str, text: str) -> None:
    p = CH3 / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + "\n", encoding="utf-8")


w(
    "3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md",
    """## 3.1 Reordering and Optimizations（重排与优化）

> 书 §3.1 · 承接 [第 2 章 Relaxed API](../Chapter-02-Atomics/2.1-atomic-load-store/2.1-atomic-load-store.md)  
> **本节正文已拆分**；按 **3.1.1 → 3.1.2** 顺序阅读。

---

## 为什么需要内存顺序？

现代 **CPU** 与 **编译器** 会做 **指令重排**。多线程下「先写数据、再写标志」可能被乱序，他线程看到标志已 set、数据仍未就绪。

**`Relaxed`** 只保证对 **该原子变量** 读写不可分割，**不**保证 `data = 42` 在 `flag.store` 之前对他线程可见 → 见 [3.3.2 Release/Acquire](../3.3-memory-order-options/3.3.2-release-acquire.md)。

---

## §3.1 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **3.1.1** | 编译器重排 | [3.1.1-compiler-reordering.md](./3.1.1-compiler-reordering.md) |
| **3.1.2** | 硬件 / CPU 重排 | [3.1.2-hardware-reordering.md](./3.1.2-hardware-reordering.md) |

**阅读顺序**：**3.1.1 → 3.1.2** → [3.2 索引](../3.2-the-memory-model/3.2-the-memory-model.md)

章索引：[本章学习笔记.md](../本章学习笔记.md)""",
)

w(
    "3.1-reordering-and-optimizations/3.1.1-compiler-reordering.md",
    """## 3.1.1 Compiler Reordering（编译器重排）

> 书 §3.1 · 索引：[3.1](./3.1-reordering-and-optimizations.md) · 后：[3.1.2 硬件重排](./3.1.2-hardware-reordering.md)

---

| 行为 | 后果 |
|------|------|
| 优化、删「无用」读写 | 单线程仍正确 |
| 调换**看似无关**语句顺序 | 多线程下可能破坏「先 data 后 flag」意图 |

**x86 不乱序 ≠ 可省略 Ordering**：**编译器仍可重排**；Rust 语义按**抽象内存模型**，必须传正确 `Ordering`。

§3.1 索引：[3.1-reordering-and-optimizations.md](./3.1-reordering-and-optimizations.md)""",
)

w(
    "3.1-reordering-and-optimizations/3.1.2-hardware-reordering.md",
    """## 3.1.2 Hardware Reordering（硬件重排）

> 书 §3.1 · 索引：[3.1](./3.1-reordering-and-optimizations.md) · 前：[3.1.1](./3.1.1-compiler-reordering.md)

---

| 机制 | 说明 |
|------|------|
| 乱序执行、Store Buffer | 写可能暂留本地，他线程不可见 |
| 缓存一致性 | 不同架构默认强弱不同（x86 较强、ARM 较弱） |

Rust 用 **内存模型 + `Ordering`** 统一跨架构行为 → [3.2 内存模型](../3.2-the-memory-model/3.2-the-memory-model.md)。

典型反例（伪代码）见 [3.1 索引](./3.1-reordering-and-optimizations.md) · 正解：**Release + Acquire**。""",
)

w(
    "3.2-the-memory-model/3.2-the-memory-model.md",
    """## 3.2 The Memory Model（内存模型）

> 书 §3.2 · 前：[3.1 重排](../3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md)  
> **本节正文已拆分**；按 **3.2.1 → 3.2.2** 顺序阅读。

---

Rust 并发内存语义**继承 C++ 内存模型**：抽象规则描述多线程下哪些写对哪些读可见。

深度附录：[Appendix A](../Appendix/A-rust-memory-model.md) · [Atomics与内存序-贯通笔记.md](../../Atomics与内存序-贯通笔记.md)

---

## §3.2 子笔记索引

| 子节 | 主题 | 笔记 |
|------|------|------|
| **3.2.1** | Happens-Before | [3.2.1-happens-before.md](./3.2.1-happens-before.md) |
| **3.2.2** | Spawning and Joining | [3.2.2-spawning-and-joining.md](./3.2.2-spawning-and-joining.md) |

**阅读顺序**：**3.2.1 → 3.2.2** → [3.3 索引](../3.3-memory-order-options/3.3-memory-order-options.md)

章索引：[本章学习笔记.md](../本章学习笔记.md)""",
)

w(
    "3.2-the-memory-model/3.2.1-happens-before.md",
    """## 3.2.1 Happens-Before Relationship

> 书 §3.2 · 索引：[3.2](./3.2-the-memory-model.md) · 后：[3.2.2 spawn/join](./3.2.2-spawning-and-joining.md)

---

若 **A happens-before B**，则 A 对内存的写对 B **可见**。

```
线程 A：写 data → Release store(flag)
              ═══════ HB ═══════
线程 B：Acquire load(flag) 读到 → 此后读 data 可见 A 的写
```

| | 原子性 | Ordering |
|---|--------|----------|
| 解决 | 单变量不可分割 | 其他内存的可见性与顺序 |

§3.2 索引：[3.2-the-memory-model.md](./3.2-the-memory-model.md)""",
)

w(
    "3.2-the-memory-model/3.2.2-spawning-and-joining.md",
    """## 3.2.2 Spawning and Joining

> 书 §3.2 · 索引：[3.2](./3.2-the-memory-model.md) · 前：[3.2.1 HB](./3.2.1-happens-before.md)

---

**`thread::spawn` 之前**的内存效果 **happens-before** 子线程开始执行；**子线程结束** **happens-before** **`join` 返回**。

因此 `join` 后读子线程写入的数据是安全的（在类型系统允许的共享方式下）。

对照 [Ch01 §1.1 spawn/join](../../Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust/1.1-threads-in-rust.md) · 原子信使仍须 **Release/Acquire**（不等同于 join 同步所有共享状态）。

§3.2 索引：[3.2-the-memory-model.md](./3.2-the-memory-model.md)""",
)

w(
    "本章学习笔记.md",
    f"""# 第三章 — 学习笔记索引（实体书 §3.1～3.6）

> **结构**：书 § = 文件夹名；每节含 **索引**、子笔记、`code/` 下 demo（如有）。书目：[全书目录-与实体书一致.md](../全书目录-与实体书一致.md)  
> **贯通**：[Atomics与内存序-贯通笔记.md](../Atomics与内存序-贯通笔记.md)

---

## 一句话

**原子性**消除该原子变量上的 DR；**Ordering** 解决其他内存的可见性与顺序。

---

## §3.1～3.6 索引

| 书 § | 笔记 | Demo |
|------|------|------|
| 3.1 | [3.1-reordering-and-optimizations.md](./3.1-reordering-and-optimizations/3.1-reordering-and-optimizations.md)（索引） | — |
| 3.1.1 | [3.1.1-compiler-reordering.md](./3.1-reordering-and-optimizations/3.1.1-compiler-reordering.md) | — |
| 3.1.2 | [3.1.2-hardware-reordering.md](./3.1-reordering-and-optimizations/3.1.2-hardware-reordering.md) | — |
| 3.2 | [3.2-the-memory-model.md](./3.2-the-memory-model/3.2-the-memory-model.md)（索引） | — |
| 3.2.1 | [3.2.1-happens-before.md](./3.2-the-memory-model/3.2.1-happens-before.md) | — |
| 3.2.2 | [3.2.2-spawning-and-joining.md](./3.2-the-memory-model/3.2.2-spawning-and-joining.md) | — |
| 3.3 | [3.3-memory-order-options.md](./3.3-memory-order-options/3.3-memory-order-options.md)（索引） | [3.3-memory-order-options/code/](./3.3-memory-order-options/code/) |
| 3.3.1 | [3.3.1-relaxed.md](./3.3-memory-order-options/3.3.1-relaxed.md) | ↑ |
| 3.3.2 | [3.3.2-release-acquire.md](./3.3-memory-order-options/3.3.2-release-acquire.md) | ↑ |
| 3.3.3 | [3.3.3-acqrel.md](./3.3-memory-order-options/3.3.3-acqrel.md) | ↑ |
| 3.3.4 | [3.3.4-seq-cst.md](./3.3-memory-order-options/3.3.4-seq-cst.md) | ↑ |
| 3.3.5 | [3.3.5-ordering-compare-select.md](./3.3-memory-order-options/3.3.5-ordering-compare-select.md) | ↑ |
| 3.4 | [3.4-fences.md](./3.4-fences/3.4-fences.md) | [3.4-fences/code/](./3.4-fences/code/) |
| 3.5 | [3.5-common-misconceptions.md](./3.5-common-misconceptions/3.5-common-misconceptions.md) | — |
| 3.6 | [3.6-summary.md](./3.6-summary/3.6-summary.md) | — |

---

## Demo 一览

| 主题 | 位置 |
|------|------|
| Relaxed / RA | [Ch02 2.1 code](../Chapter-02-Atomics/2.1-atomic-load-store/code/) |
| SeqCst 等 | [3.3-memory-order-options/code/](./3.3-memory-order-options/code/) |
| Fence | [3.4-fences/code/](./3.4-fences/code/) |
| 自旋锁 RA 样板 | [Ch04 4.1](../Chapter-04-Spin-Locks/4.1-minimal-implementation/) |

```bash
cargo build --manifest-path {M}
```""",
)

print("wrote ch03 section notes + index")
