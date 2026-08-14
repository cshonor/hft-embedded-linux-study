# 第 7 章：宏 (Macros)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 原书章名：**Macros** — 让编译器替你生成代码：`macro_rules!` 声明宏 vs 过程宏 (proc-macros)。

## 本章结构（与原书对齐）

**开篇总览** · **3 个主节** · 连同二级子节共 **10 个部分**（2 个带子的主节标题 + 3 + 4 + Summary）。

| | 笔记 |
|---|------|
| **总览（复习）** | [00 宏核心总览](./00-macros-overview.md) — hub · **§0 直觉** |
| **00.1 Token** | [00-1 Token 与宏展开](./00-1-token-and-macro-pipeline.md) |
| **00.2 分类** | [00-2 宏分类总览](./00-2-macro-taxonomy.md) |
| **00.3 宏 vs 函数** | [00-3 完整对比](./00-3-macro-vs-function.md) |

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Declarative Macros | [01 何时使用](./01-when-to-use-declarative-macros.md)（削样板 · 决策树）· [02 如何工作](./02-how-declarative-macros-work.md) · [03 如何编写](./03-how-to-write-declarative-macros.md)（片段分类符） |
| **2** | Procedural Macros | [04 过程宏类型](./04-types-of-procedural-macros.md)（derive/属性/类函数）· [05 代价](./05-cost-of-procedural-macros.md) · [06 你真的需要宏吗](./06-so-you-think-you-want-a-macro.md) · [07 如何工作](./07-how-procedural-macros-work.md) |
| **3** | Summary | [08 小结](./08-summary.md) |

## 阅读顺序

```text
00（hub）→ 00-1 / 00-2 / 00-3（按需）→ 01 → 02 → … → 08
```

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| 宏入门 | [19.5 宏](../../00-Book/19-advanced-features/19.5-宏.md) |
| 可运行 demo | [19.5-macros-demo](../../00-Book/19-advanced-features/19.5-macros-demo/) |
| 展开宏排错 | [第 13 章 · cargo-expand](../Chapter-13-Rust-Ecosystem/01-tools.md) |
| **过程宏动手** | [**proc-macro-workshop 实验**](./proc-macro-workshop-lab.md) → [dtolnay/proc-macro-workshop](https://github.com/dtolnay/proc-macro-workshop) |

## 旧版单文件

早期合并稿已拆入上表各文件；见 git 中的 `7-宏-Macros-深度解析.md`。
