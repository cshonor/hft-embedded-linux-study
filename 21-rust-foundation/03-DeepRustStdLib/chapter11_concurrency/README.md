# 第 11 章 · 并发编程

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 10 章 进程管理](../chapter10_process_management/README.md) · 后：[第 12 章 文件系统](../chapter12_filesystem/README.md) · 原书目录：[本书目录 § 第 11 章](../本书目录.md#第-11-章--并发编程)

**本章定位**：**STD 并发核心**（原书 p.264+）— **Futex** · **Mutex/Condvar/RwLock/Barrier**（三层架构）· **Once/OnceLock/LazyLock** · **thread** · **MPSC** · **极简 RUNTIME** · **[11.12/11.13 源码附录](./11.12-mutex-futex-walkthrough.md)**。

**原书主线**：Futex 底座 → 共享内存同步 → 单次 init → 线程 → **通信优于共享**（MPSC）→ 无 GC Runtime。

**阅读顺序**：**[11.0 总览](./11.0-chapter-overview.md) → 11.1 → … → 11.11**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **11.0** | 本章总览 · 三层架构 | [笔记](./11.0-chapter-overview.md) |
| **11.1** | Futex 分析 | [笔记](./11.1-futex.md) |
| **11.2** | Mutex<T> 类型分析 | [笔记](./11.2-mutex-overview.md) |
| **11.2.1** | OS 相关适配层 | [笔记](./11.2.1-mutex-os-layer.md) |
| **11.2.2** | OS 无关适配层 | [笔记](./11.2.2-mutex-os-agnostic.md) |
| **11.2.3** | 对外接口层 | [笔记](./11.2.3-mutex-public-api.md) |
| **11.3** | Condvar 类型分析 | [笔记](./11.3-condvar-overview.md) |
| **11.3.1** | OS 相关适配层 | [笔记](./11.3.1-condvar-os-layer.md) |
| **11.3.2** | OS 无关适配层 | [笔记](./11.3.2-condvar-os-agnostic.md) |
| **11.3.3** | 对外接口层 | [笔记](./11.3.3-condvar-public-api.md) |
| **11.4** | RwLock<T> 类型分析 | [笔记](./11.4-rwlock-overview.md) |
| **11.4.1** | OS 相关适配层 | [笔记](./11.4.1-rwlock-os-layer.md) |
| **11.4.2** | OS 无关适配层 | [笔记](./11.4.2-rwlock-os-agnostic.md) |
| **11.4.3** | 对外接口层 | [笔记](./11.4.3-rwlock-public-api.md) |
| **11.5** | Barrier 类型分析 | [笔记](./11.5-barrier.md) |
| **11.6** | Once 类型分析 | [笔记](./11.6-once.md) |
| **11.7** | OnceLock<T> 类型分析 | [笔记](./11.7-oncelock.md) |
| **11.8** | LazyLock<T> 类型分析 | [笔记](./11.8-lazylock.md) |
| **11.9** | 线程分析 | [笔记](./11.9-thread-overview.md) |
| **11.9.1** | OS 相关适配层 | [笔记](./11.9.1-thread-os-layer.md) |
| **11.9.2** | OS 无关适配层 | [笔记](./11.9.2-thread-os-agnostic.md) |
| **11.9.3** | 对外接口层 | [笔记](./11.9.3-thread-public-api.md) |
| **11.10** | 线程消息通信——MPSC | [笔记](./11.10-mpsc-overview.md) |
| **11.10.1** | 消息队列类型——Queue<T> | [笔记](./11.10.1-mpsc-queue.md) |
| **11.10.2** | 阻塞及唤醒信号机制 | [笔记](./11.10.2-mpsc-block-wake.md) |
| **11.10.3** | 一次性通信通道机制 | [笔记](./11.10.3-mpsc-oneshot.md) |
| **11.10.4** | Shared 类型通道 | [笔记](./11.10.4-mpsc-shared.md) |
| **11.10.5** | 对外接口层 | [笔记](./11.10.5-mpsc-public-api.md) |
| **11.11** | Rust 的 RUNTIME | [笔记](./11.11-runtime.md) |
| **11.12** | Mutex futex 走读（附录） | [笔记](./11.12-mutex-futex-walkthrough.md) |
| **11.13** | MPSC 队列走读（附录） | [笔记](./11.13-mpsc-queue-walkthrough.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **11.0** | 本章总览 | ✅ |
| **11.1** | Futex 分析 | ✅ |
| **11.2** | `Mutex<T>` 类型分析 | ✅ |
| **11.2.1** | OS 相关适配层 | ✅ |
| **11.2.2** | OS 无关适配层 | ✅ 脉络 |
| **11.2.3** | 对外接口层 | ✅ |
| **11.3** | `Condvar` 类型分析 | ✅ |
| **11.3.1** | OS 相关适配层 | ✅ 脉络 |
| **11.3.2** | OS 无关适配层 | ✅ 脉络 |
| **11.3.3** | 对外接口层 | ✅ |
| **11.4** | `RwLock<T>` 类型分析 | ✅ |
| **11.4.1** | OS 相关适配层 | ✅ 脉络 |
| **11.4.2** | OS 无关适配层 | ✅ 脉络 |
| **11.4.3** | 对外接口层 | ✅ |
| **11.5** | `Barrier` 类型分析 | ✅ |
| **11.6** | `Once` 类型分析 | ✅ |
| **11.7** | `OnceLock<T>` 类型分析 | ✅ |
| **11.8** | `LazyLock<T>` 类型分析 | ✅ |
| **11.9** | 线程分析 | ✅ |
| **11.9.1** | OS 相关适配层 | ✅ |
| **11.9.2** | OS 无关适配层 | ✅ 脉络 |
| **11.9.3** | 对外接口层 | ✅ |
| **11.10** | 线程消息通信——MPSC | ✅ |
| **11.10.1** | 消息队列类型——`Queue<T>` | ✅ |
| **11.10.2** | 阻塞及唤醒信号机制 | ✅ |
| **11.10.3** | 一次性通信通道机制 | ✅ 脉络 |
| **11.10.4** | `Shared` 类型通道 | ✅ 脉络 |
| **11.10.5** | 对外接口层 | ✅ |
| **11.11** | Rust 的 RUNTIME | ✅ |
| **11.12** | Mutex futex 走读 | ✅ |
| **11.13** | MPSC 队列走读 | ✅ |

↔ [8.5 · Arc](../chapter08_smart_pointers/8.5-arc-overview.md) · [10.3 · 进程三层](../chapter10_process_management/10.3-process-mgmt.md)

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| OS 线程基础 | [05-atomic Ch1](../../05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/) |
| RwLock 体系 | [RwLock 贯通笔记](../../05-Async-Concurrency-Network/01-atomic/RwLock与读写锁体系-贯通笔记.md) |
| 异步 RUNTIME | [05-async](../../05-Async-Concurrency-Network/02-async/) · [RFR Ch10](../../02-RFR/Chapter-10-Asynchronous-Programming/README.md) |
| `Arc` | [第 8 章 §8.5](../chapter08_smart_pointers/README.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **11.1～11.4** | 订单簿 / 持仓互斥 vs 读写锁；锁竞争与 futex 唤醒成本 |
| **11.7～11.8** | 单例配置、交易所规则表延迟初始化 |
| **11.10** | 行情线程 → 策略线程 MPSC；与无锁队列选型对照 |
| **11.11** | `std` 线程模型 vs `tokio` 运行时边界 |
