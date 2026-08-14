# 2.2 Allocations（分配与释放）

> 所属：**Types Across Language Boundaries** · [← 章索引](./README.md)

## 谁分配谁释放

| 模式 | 规则 |
|------|------|
| **库内分配** | C `xxx_new` / `xxx_free` → Rust `Drop` 调 `free` |
| **调用方 buffer** | Rust 分配 + 长度 → C **填充**；明确边界与初始化 |

## 禁止

- Rust 侧 `free` C 未约定可 `free` 的指针
- 双重释放、泄漏

## RAII 包装

安全层用 **`struct` + `Drop`** 唯一负责释放路径 → [第 1 章 Drop](../Chapter-01-Foundations/04-ownership.md)

ER → [Item 34 demo](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo/)
