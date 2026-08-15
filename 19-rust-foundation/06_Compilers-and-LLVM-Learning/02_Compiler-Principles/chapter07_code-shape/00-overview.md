# 第 7 章 · 代码形态 · 本章定位

← [本章目录](./README.md) · 上一章：[ch6 过程抽象](../chapter06_procedures/README.md) · 下一节：[01-storage-locations.md](./01-storage-locations.md)

---

```text
ch5 IR「长什么样」  +  ch6 运行时「怎么活」
         ↓
    ch7 源语言结构 → 具体 IR 指令序列（代码形态）
         ↓
    ch8～10 优化 · ch11～13 代码生成
```

| 概念 | 含义 |
|------|------|
| **Code Shape** | 同一源结构（如 `a[i]+1`）的**一种** lowering 选择 |
| **Trade-off** | 形态不同 → 优化器能做什么、后端生成多快/多好 |

**Part II 基础结构（ch5～7）在本章收尾**；下一部分 **Part III 优化（ch8）**。

与 [ch1 实用性原则](../chapter01_overview/02-two-fundamental-principles.md) — 代码形态是「实用性」在 lowering 层的体现。
