# 第 8 章 · Statements and State（语句与状态） · §8.1 语句（Statements）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-variable-declarations.md](./02-variable-declarations.md)

---

| 对比 | 表达式 | 语句 |
|------|--------|------|
| 主要目的 | 产生**值** | 产生**副作用**（I/O、改状态） |

新增 **`Stmt`** AST 层次（可扩展 `GenerateAst` 或手写）：

| 语句 | 形式 |
|------|------|
| **Expression stmt** | `expr;` |
| **Print** | `print expr;` |

- Parser 解析 **statement 列表**。
- Interpreter 实现 **`Stmt.Visitor`**（与 `Expr.Visitor` 并列）。

---
