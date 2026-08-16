# 第 3 章 · The Lox Language（Lox 语言概览） · §3.5 语句（Statements）

← [本章目录](./README.md) · 上一节：[04-expressions.md](./04-expressions.md) · 下一节：[06-variables.md](./06-variables.md)

---

| 概念 | 说明 |
|------|------|
| **表达式** | 主要**产生一个值** |
| **语句** | 主要产生**副作用（Effect）**（改状态、I/O 等） |

Lox 语句形态：

| 语句 | 形式 |
|------|------|
| 打印 | `print expr;` |
| 表达式语句 | `expr;` |
| 代码块 | `{ ... }` → 影响**作用域** |

**Design Note（章末）**：*Expressions and Statements* —— 表达式 vs 语句的划分（不计入 30 章正文，与 §3.4～3.5 呼应）。

---
