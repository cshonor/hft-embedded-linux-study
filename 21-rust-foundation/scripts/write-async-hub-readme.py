#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HUB = ROOT / "05-Async-Concurrency-Network/README.md"

HUB.write_text("""# 04 · Async · Concurrency · Network

> 所属：Rust 主线 **`00-Book` → `02-RFR` → `01-ER` → `04-Rust-Nomicon`** 之后的**实战专题**  
> 下一专题：[05 · Compilers & LLVM Learning](../06_Compilers-and-LLVM-Learning/README.md)

**Rust 基础 → 内存黑魔法（Nomicon）→ 并发 / 异步 / 网络 → LLVM / IR** — 本目录按 **01 → 02 → 03** 编号，对应同步并发 → 异步运行时 → 网络落地，**步步依赖、不宜跳读**。

---

## 学习链路（固定顺序）

```text
04-Rust-Nomicon 通读（unsafe / 布局 / Send·Sync 边界）
        ↓
01-atomic/                 狗熊书 · 原子、锁、内存序、无锁
        ↓
02-async_tokio/            Async Rust · Future / 运行时 / 设计模式
        ↓
03-rust_network_programming/   阻塞 Socket → Tokio 网络 → HTTP / 安全
        ↓
06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17   用上面代码反查 IR / 优化
```

| # | 目录 | 书目 | 为何在这一步 |
|:---:|------|------|--------------|
| **01** | [`01-atomic/`](./01-atomic/README-学习区.md) | *Rust Atomics and Locks* | 线程、锁、内存序 — async / 网络里的 `Arc`/`Mutex`/原子都建立在这 |
| **02** | [`02-async_tokio/`](./02-async_tokio/README.md) | *Async Rust* | 在 01 的并发模型上理解 Future、Pin、executor；再读网络异步才不空转 |
| **03** | [`03-rust_network_programming/`](./03-rust_network_programming/README.md) | *Network Programming with Rust* | 把 01/02 落到 Socket、协议、Tokio 网络实战 |

LLVM 取舍 → [Learn LLVM 17 学习取舍](../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md)

---

## 运行 demo（仓库根）

```bash
cargo build --manifest-path 05-Async-Concurrency-Network/01-atomic/Cargo.toml
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_1_tcp_echo_server
```

| 目录 | 结构 | 说明 |
|------|------|------|
| **01-atomic/** | `Chapter-01`～`10` + `study_atomic` | 见 [README-学习区](./01-atomic/README-学习区.md) |
| **02-async_tokio/** | `ch01`～`ch11` · `X.Y-slug.md` + demo | 见 [README](./02-async_tokio/README.md) |
| **03-rust_network_programming/** | `stage01`～`09` | 见 [README](./03-rust_network_programming/README.md) |

共用规范：[01-atomic/小节笔记与Demo规范.md](./01-atomic/小节笔记与Demo规范.md) · [03-rust_network_programming/小节笔记与Demo规范.md](./03-rust_network_programming/小节笔记与Demo规范.md)

---

## 与 RFR / Nomicon / LLVM 对照

| 本目录 | RFR | Nomicon | LLVM（05） |
|--------|-----|---------|------------|
| 01-atomic | [Ch10](../02-RFR/Chapter-10-Concurrency-and-Parallelism/README.md) | [Ch07](../04-Rust-Nomicon/07_Concurrency_Atomic/README.md) | `ir_samples/atomic_ir/` |
| 02-async_tokio | [Ch08](../02-RFR/Chapter-08-Asynchronous-Programming/README.md) | Pin / Send | `ir_samples/async_tokio_ir/` |
| 03-rust_network stage07 | Ch08 + Ch10 | — | `ir_samples/network_ir/` |

章节对照 → [02-RFR/学习路径与章节对照.md](../02-RFR/学习路径与章节对照.md)

---

## 速记

**顺序** 01-atomic → 02-async_tokio → 03-rust_network → **05** LLVM IR · **勿跳步**
""", encoding="utf-8", newline="\n")
print("hub ok")
