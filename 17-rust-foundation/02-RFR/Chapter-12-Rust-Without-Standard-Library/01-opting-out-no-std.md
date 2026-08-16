# 1. Opting Out of the Standard Library（退出 std）

> [← 章索引](./README.md)

## 层次

| Crate | 假设 |
|-------|------|
| **`core`** | 平台无关子集；**不假设堆** |
| **`alloc`** | 需全局分配器 |
| **`std`** | OS 能力（I/O、线程、默认堆等） |

## `#![no_std]`

crate 根属性 — 不链接完整 `std`；预导入走 **`core`** prelude（随 edition 而变）。

## 库常见模式

默认 **`no_std`** + **`feature = "std"`** 在宿主打开扩展 — 兼顾嵌入式与桌面用户。

ER → [Item 33](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md)
