# 第 8 章 · Statements and State（语句与状态） · §8.2 变量声明（Variable Declarations）

← [本章目录](./README.md) · 上一节：[01-statements.md](./01-statements.md) · 下一节：[03-environments.md](./03-environments.md)

---

- **`var name = init;`** 或 **`var name;`**（无 init → **`nil`**）。
- 语法上区分 **declaration** vs 普通 **statement**（作用域规则不同）。
- AST：如 **`Stmt.Var`**（name + initializer 表达式）。

---
