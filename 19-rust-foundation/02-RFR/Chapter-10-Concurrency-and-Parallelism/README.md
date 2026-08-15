# 第 10 章：并发与并行 (Concurrency and Parallelism)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 架构与底层之间：并发**方法论** — 模型取舍、async 与多核、原子与 memory ordering。

## 本章结构（与原书对齐）

**6 个主节** · 连同二级子节共 **19 个部分**（4 个带子的主节标题 + 2 + 3 + 1 + 5 + 3 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | The Trouble with Concurrency | [01 正确性](./01-correctness.md) · [02 性能](./02-performance.md) |
| **2** | Concurrency Models | [03 共享内存](./03-shared-memory.md) · [04 工作池](./04-worker-pools.md) · [05 Actor](./05-actors.md) |
| **3** | Asynchrony and Parallelism | [06 异步与并行](./06-asynchrony-parallelism.md) |
| **4** | Lower-Level Concurrency | [07 内存操作](./07-memory-operations.md) · [08 原子类型](./08-atomic-types.md) · [09 内存序](./09-memory-ordering.md) · [10 CAS](./10-compare-exchange.md) · [11 Fetch 方法](./11-fetch-methods.md) |
| **5** | Sane Concurrency | [12 从简单开始](./12-start-simple.md) · [13 压力测试](./13-stress-tests.md) · [14 并发测试工具](./14-concurrency-testing-tools.md) |
| **6** | Summary | [15 小结](./15-summary.md) |

## 与 The Book / atomic / ER 对照

| 主题 | 本仓库 |
|------|--------|
|  fearless concurrency | [16 章](../../00-Book/16-fearless-concurrency/) |
| Send/Sync | [16.4](../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md) |
| 原子 / 内存序 | `05-Async-Concurrency-Network/01-atomic/` · [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/src/lib.rs) |
| ER Item 17 | [共享状态并行](../../01-ER/Chapter-03-Concepts/Item-17-shared-state-parallelism/README.md) |

## 旧版单文件

见 git 中的 `10-并发与并行-Concurrency-and-Parallelism-深度解析.md`。
