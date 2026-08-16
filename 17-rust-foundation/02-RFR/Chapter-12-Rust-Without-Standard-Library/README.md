# 第 12 章：脱离标准库的 Rust (`no_std`)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 裸机 / 嵌入式 / 自研 OS — 无完整 OS 时仍用 **`core` / `alloc`** 写可维护 Rust。

## 本章结构（与原书对齐）

**7 个主节** · 连同二级子节共 **10 个部分**（1 个带子的主节标题 + 3 + 6 个无子主节含 Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Opting Out of the Standard Library | [01 退出 std](./01-opting-out-no-std.md) |
| **2** | Dynamic Memory Allocation | [02 动态分配](./02-dynamic-memory-allocation.md) |
| **3** | The Rust Runtime | [03 Panic](./03-panic-handler.md) · [04 初始化](./04-program-initialization.md) · [05 OOM](./05-oom-handler.md) |
| **4** | Low-Level Memory Accesses | [06 底层内存访问](./06-low-level-memory-access.md) |
| **5** | Misuse-Resistant Hardware Abstraction | [07 硬件抽象](./07-hardware-abstraction.md) |
| **6** | Cross-Compilation | [08 交叉编译](./08-cross-compilation.md) |
| **7** | Summary | [09 小结](./09-summary.md) |

## 与 ER / Nomicon 对照

| 主题 | 本仓库 |
|------|--------|
| no_std | [ER Item 33](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md) · [demo](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/) |
| Nomicon | [04-Rust-Nomicon](../../04-Rust-Nomicon/README.md) |

## 旧版单文件

见 git 中的 `12-脱离标准库-no_std-深度解析.md`。
