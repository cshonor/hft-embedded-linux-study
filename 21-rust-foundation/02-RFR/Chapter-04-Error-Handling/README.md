# 第 4 章：错误处理 (Error Handling)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 失败是 **API 契约** 的一部分：如何表示、如何传播、调用者如何交互。

## 本章结构（与原书对齐）

**3 个主节** · 连同二级子节共 **6 个部分**（1 个带子的主节标题 + 3 + 1 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Representing Errors | [01 枚举式错误](./01-enumeration.md) · **[01 错误链 source / thiserror / anyhow](./01-error-source-chain.md)** · [02 不透明错误](./02-opaque-errors.md) · [03 特殊情形](./03-special-error-cases.md) |
| **2** | Propagating Errors | [04 传播错误](./04-propagating-errors.md) |
| **3** | Summary | [05 小结](./05-summary.md) |

## 跨章索引（线程 × 错误链）

| 层 | 场景 | 笔记 |
|----|------|------|
| **包装层** | `Result` + `?`、`thiserror`、`source()` | [01 错误链](./01-error-source-chain.md) |
| **容器层** | 子线程 panic → `join` → `Box<dyn Any>` / downcast | [1.1.2.3](../../05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust/1.1.2.3-panic-box-dyn-any.md) |

传播小节亦指向 1.1.2.3 → [04 传播错误](./04-propagating-errors.md)。

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| `Result`、`?` | [9.2 Result](../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) |
| panic 策略 | [9.1 / 9.3](../../00-Book/09-error-handling/9.1-panic-与不可恢复的错误.md) |
| 惯用错误类型 | [ER Item 04](../../01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/README.md) |
| `thiserror` / `anyhow` | [Item 04 demo](../../01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/demo/) |

## 旧版单文件

见 git 中的 `4-错误处理-Error-Handling-深度解析.md`。
