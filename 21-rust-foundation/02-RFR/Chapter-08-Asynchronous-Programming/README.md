# 第 8 章：异步编程 (Asynchronous Programming)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> `async/await` 背后的**状态机、`Pin`、`Poll`/`Waker`、spawn** — 内功向，不只讲某个 executor API。

## 本章结构（与原书对齐）

**5 个主节** · 连同二级子节共 **15 个部分**（3 个带子的主节标题 + 4 + 2 + 4 + 1 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | What's the Deal with Asynchrony? | [01 同步接口](./01-synchronous-interfaces.md) · [02 多线程](./02-multithreading.md) · [03 异步接口](./03-asynchronous-interfaces.md) · [04 标准化轮询](./04-standardized-polling.md) |
| **2** | Ergonomic Futures | [05 async/await](./05-async-await.md) · [06 Pin/Unpin](./06-pin-unpin.md) |
| **3** | Going to Sleep | [07 唤醒](./07-waking-up.md) · [08 Poll 契约](./08-poll-contract.md) · [09 唤醒的误称](./09-waking-misnomer.md) · [10 任务与子执行器](./10-tasks-subexecutors.md) |
| **4** | Tying It All Together with spawn | [11 spawn](./11-spawn.md) |
| **5** | Summary | [12 小结](./12-summary.md) |

## 与仓库对照

| 主题 | 本仓库 |
|------|--------|
| async / Future | [RFR 学习路径 · 第 8 章](../学习路径与章节对照.md) |
| [`05-Async-Concurrency-Network/02-async_tokio/`](../../05-Async-Concurrency-Network/02-async_tokio/README.md) | 05 专题 · async 实战（Nomicon 后推进） |
| Pin 报错 | 本章 [06](./06-pin-unpin.md) |
| 并发模型 | [第 10 章](../Chapter-10-Concurrency-and-Parallelism/README.md) |

## 旧版单文件

见 git 中的 `8-异步编程-Asynchronous-Programming-深度解析.md`。
