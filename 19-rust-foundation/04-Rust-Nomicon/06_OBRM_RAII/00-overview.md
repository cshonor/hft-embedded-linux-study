# 06 · The Perils Of OBRM · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[05 Uninit Mem](../05_Uninit_Mem/README.md) · 下一章：[07 Concurrency](../07_Concurrency_Atomic/README.md)

---

官方标题 **The Perils Of Ownership Based Resource Management (OBRM / RAII)**。获取资源常伴随对象创建，释放依赖销毁。本章探讨该机制在底层编程中的**挑战与风险**。

> **先读**：[00.1 RAII 与 OBRM 辨析](./00-1-RAII与OBRM辨析.md)（概念）→ 完整版 [Book 15.3.0](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) · 再读本章 01–05（陷阱）

| 对照 | 路径 |
|------|------|
| Drop / RAII / OBRM 辨析 | [Book 15.3.0 RAII/OBRM](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) · [15.3 Drop](../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) |
| ManuallyDrop | [15.3.1](../../00-Book/15-smart-pointers/15.3.1-Drop顺序与进阶场景.md) |
| ER Item 11 | [drop-raii](../../01-ER/Chapter-02-Traits/Item-11-drop-raii/README.md) |
| catch_unwind | [09_FFI](../09_FFI/00-overview.md) §7 |

**读完应能回答**：Rust 构造/析构与 C++ 有何不同、为何 `forget` 仍算内存安全、代理类型泄漏为何危险、Guard 如何保异常安全。

---

## 小节路线图

```text
00.1  RAII / OBRM 概念（Perils 前置）→ Book 15.3.0 完整版
  ↓
01  极简构造 · 递归 Drop → construct_drop.rs
  ↓
02  forget 与泄漏 → forget_leak.rs
  ↓
03  代理类型（Drain / Rc / scoped）→ proxy_types.rs
  ↓
04  panic Guard 异常安全 → panic_guard.rs
  ↓
05  Mutex 投毒 → poisoning.rs
  ↓
07 Concurrency
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| **0.1** | **RAII / OBRM 辨析** | [00-1-RAII与OBRM辨析.md](./00-1-RAII与OBRM辨析.md) · [Book 15.3.0](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) |
| 1 | 构造与析构 | [01-construct-drop.md](./01-construct-drop.md) |
| 2 | 泄漏与 forget | [02-forget-leak.md](./02-forget-leak.md) |
| 3 | 代理类型 | [03-proxy-types.md](./03-proxy-types.md) |
| 4 | 栈展开与异常安全 | [04-unwinding.md](./04-unwinding.md) |
| 5 | 数据投毒 | [05-poisoning.md](./05-poisoning.md) |
| — | 速记 · 自测 |

---

## 一句话

**OBRM 风险章** — 极简构造与递归 Drop、`mem::forget` 与泄漏、代理类型陷阱、panic Guard、Mutex 投毒。

→ 从 [01-construct-drop.md](./01-construct-drop.md) 起读；源码从各节链到 `src/`。
