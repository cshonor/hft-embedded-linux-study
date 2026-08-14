# 07 · Concurrency and Parallelism · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[06 OBRM](../06_OBRM_RAII/README.md) · 下一章：[08 Impl Vec/Arc](../08_Impl_Vec_Arc/README.md)

---

官方标题 **Concurrency and Parallelism**。Rust 不预设「消息传递 vs 绿色线程」等底层立场，但类型系统使**线程安全抽象**相对易写。本章三主题：数据竞争、Send/Sync、原子与内存序。

| 对照 | 路径 |
|------|------|
| Send / Sync | [Book 16.4 Send与Sync](../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md) |
| RFR 并发 | [Ch10 Concurrency](../../02-RFR/Chapter-10-Concurrency-and-Parallelism/README.md) |
| 实现 Arc（后续） | [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md) |

**读完应能回答**：数据竞争 vs 竞争条件、Send/Sync 含义、Relaxed/Acquire-Release/SeqCst 如何选用。

---

## 小节路线图

```text
01  数据竞争 vs 竞争条件 · TOCTOU → data_races.rs
  ↓
02  Send / Sync 标记 → send_sync.rs
  ↓
03  原子与内存序 → atomics.rs
  ↓
08 Impl Vec/Arc
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 数据竞争 | [01-data-races.md](./01-data-races.md) |
| 2 | Send / Sync | [02-send-sync.md](./02-send-sync.md) |
| 3 | 原子操作 | [03-atomics.md](./03-atomics.md) |
| — | 速记 · 自测 |

---

## 一句话

**并发章** — Safe 消除数据竞争但不防死锁；Send/Sync 标记；C++20 内存序与 Relaxed / Acquire-Release / SeqCst。

→ 从 [01-data-races.md](./01-data-races.md) 起读；源码从各节链到 `src/`。
