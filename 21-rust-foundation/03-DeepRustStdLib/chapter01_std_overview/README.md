# 第 1 章 · Rust 标准库体系概述

> 所属：[03 DeepRustStdLib](../README.md) · 后：[第 2 章 Rust 特征小议](../chapter02_rust_features_summary/README.md)

**本章目标**：建立 **`core` → `alloc` → `std`** 三层架构 — Rust 为 **OS 内核与用户态** 拆分标准库；弄清每层 **定位、核心内容、能否脱离 OS**（对应原书第 1 章，见 [本书目录 § 第 1 章](../本书目录.md#第-1-章--rust-标准库体系概述p1)）。

**原书主线**：`core`（intrinsics · trait · mem/ptr · Option/Result）→ `alloc`（堆 · Allocator · 智能指针/集合）→ `std`（**SYSCALL** · 线程/文件/网络 · **仅用户态**）。

---

## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **1.1** | **`core` 库**（语言核心）：intrinsics · trait · mem/ptr · 内核/用户态均可 | [1.1-core-crate.md](./1.1-core-crate.md) |
| **1.2** | **`alloc` 库**（堆与集合）：Allocator · Box/Rc/Arc/Vec/BTree… · 仅依赖 core | [1.2-alloc-crate.md](./1.2-alloc-crate.md) |
| **1.3** | **`std` 库**（用户态）：SYSCALL · 线程/文件/网络 · 整合 core+alloc | [1.3-std-crate.md](./1.3-std-crate.md) |
| **1.4** | **回顾**：三层依赖与适用场景 | [1.4-recap.md](./1.4-recap.md) |

**阅读顺序**：**1.1 → 1.2 → 1.3 → 1.4**（自底向上）

---

## 与主线对照

| 本章概念 | 本仓库延伸 |
|----------|------------|
| `core` / 无堆 | [RFR Ch12 no_std](../../02-RFR/Chapter-12-Rust-Without-Standard-Library/README.md) · [Nomicon 10_NoStd](../../04-Rust-Nomicon/10_NoStd/README.md) |
| `alloc` / 自定义分配器 | [Nomicon 08 Vec](../../04-Rust-Nomicon/08_Impl_Vec_Arc/README.md) |
| `std` 并发 / IO | [05-atomic](../../05-Async-Concurrency-Network/01-atomic/README-学习区.md) · [05-rust_network](../../05-Async-Concurrency-Network/03-rust_network_programming/README.md) |
| 读源码方法、设计哲学 | 见 [第 2 章附录](../chapter02_rust_features_summary/README.md#附录)（与「特性小结」配套，不属本章三节） |
