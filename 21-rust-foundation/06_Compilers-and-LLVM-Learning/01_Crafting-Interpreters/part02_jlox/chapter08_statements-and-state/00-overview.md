# 第 8 章 · Statements and State（语句与状态） · 本章定位

← [本章目录](./README.md) · 下一节：[01-statements.md](./01-statements.md)

---

ch7 像**计算器**（单次表达式）；ch8 引入 **语句 + 变量** → 解释器有**记忆（状态）**。

| 概念 | ch7 | ch8 |
|------|-----|-----|
| 目的 | 表达式 **求值** | 语句 **副作用** |
| AST | `Expr` | **`Stmt`** + `Expr.Assign` 等 |
| 存储 | — | **`Environment`**（HashMap） |

| 小节 | 主题 |
|------|------|
| **§8.1** | `Stmt` · 表达式语句 · `print` |
| **§8.2** | `var` 声明 · 默认 `nil` |
| **§8.3** | `Environment` · define / lookup |
| **§8.4** | 赋值 `=` · l-value · `Expr.Assign` |
| **§8.5** | `{}` 块 · **环境链** · shadowing |

---
