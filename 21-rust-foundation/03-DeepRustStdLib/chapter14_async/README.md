# 第 14 章 · 异步编程

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 13 章 I/O 系统](../chapter13_io/README.md) · 原书目录：[本书目录 § 第 14 章](../本书目录.md#第-14-章--异步编程)

**本章定位**：全书**压轴** — **`async` 状态机 · epoll/kqueue/IOCP · Future/Poll/Waker/Context · Pin（Ch7）** · **[14.3～14.5 工具与走读](./14.3-std-future-utilities.md)**；Executor 在 **tokio/async-std**。

**原书主线**：14.1 协程+多路复用 → 14.2 Future 轮询与唤醒 → 全书收束。

**阅读顺序**：**[14.0 总览](./14.0-chapter-overview.md) → 14.1 → 14.2 → 14.3**

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **14.0** | 本章总览 · 全书收束 | [笔记](./14.0-chapter-overview.md) |
| **14.1** | Rust 协程框架简析 | [笔记](./14.1-coroutine-framework.md) |
| **14.1.1** | 协程概述 | [笔记](./14.1.1-coroutine-overview.md) |
| **14.1.2** | Rust 的 I/O 多路复用 | [笔记](./14.1.2-io-multiplexing.md) |
| **14.2** | Rust 协程支持类型简析 | [笔记](./14.2-coroutine-types.md) |
| **14.2.1** | Rust 协程管理 | [笔记](./14.2.1-coroutine-management.md) |
| **14.2.2** | Future Trait 分析 | [笔记](./14.2.2-future-trait.md) |
| **14.3** | std Future 工具 | [笔记](./14.3-std-future-utilities.md) |
| **14.4** | async 脱糖 + Pin 走读 | [笔记](./14.4-async-desugar-pin-walkthrough.md) |
| **14.5** | Waker/poll 走读 | [笔记](./14.5-waker-poll-walkthrough.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **14.0** | 本章总览 | ✅ |
| **14.1** | Rust 协程框架简析 | ✅ |
| **14.1.1** | 协程概述 | ✅ |
| **14.1.2** | Rust 的 I/O 多路复用 | ✅ |
| **14.2** | Rust 协程支持类型简析 | ✅ |
| **14.2.1** | Rust 协程管理 | ✅ |
| **14.2.2** | `Future` Trait 分析 | ✅ |
| **14.3** | std Future 工具 | ✅ |
| **14.4** | async 脱糖 + Pin 走读 | ✅ |
| **14.5** | Waker/poll 走读 | ✅ |

---

## 全书 14 章脉络（宏观）

| 段 | 章 | 主题 |
|:--:|:---:|------|
| **底座** | 1～2 | core/alloc/std · Trait · 解封装 |
| **内存** | 3 | 裸指针 · MaybeUninit · 堆 · 型变 |
| **语言根基** | 4～6 | 类型 · 迭代 · NonZero · str/slice |
| **抽象** | 7～8 | 内部可变 · **Pin** · Box/Vec/Rc/Arc |
| **OS** | 9～10 | FFI · SYSCALL · fd · 进程 |
| **并发 I/O** | 11～13 | 锁 · MPSC · fs · Read/Write/net |
| **终局** | **14** | **Future · async · 多路复用** |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| `std` RUNTIME | [11.11 RUNTIME](../chapter11_concurrency/11.11-runtime.md) |
| 阻塞 I/O | [第 13 章](../chapter13_io/README.md) |
| **Pin** | [7.4 Pin/Unpin](../chapter07_interior_mutability/7.4-pin-unpin.md) |
| RFR 异步 | [RFR Ch10](../../02-RFR/Chapter-10-Asynchronous-Programming/README.md) |
| `tokio` 实战 | [05-async](../../05-Async-Concurrency-Network/02-async/) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **14.1.2** | epoll/kqueue 与单线程多连接 |
| **14.2.2** | 少 poll/wake · 与低延时策略 |
| 全书 | std 深挖 → 实盘接 **05-async / tokio** |

---

## 延伸深化（可选）

| 方向 | 笔记入口 |
|------|----------|
| async 状态机脱糖 | [14.4](./14.4-async-desugar-pin-walkthrough.md) |
| Pin/自引用 | [14.4](./14.4-async-desugar-pin-walkthrough.md) · [7.4](../chapter07_interior_mutability/7.4-pin-unpin.md) |
| Waker 原子唤醒 | [14.5](./14.5-waker-poll-walkthrough.md) |
| Mutex vs tokio::Mutex | [14.5 §E](./14.5-waker-poll-walkthrough.md) · [11.12](../chapter11_concurrency/11.12-mutex-futex-walkthrough.md) |
