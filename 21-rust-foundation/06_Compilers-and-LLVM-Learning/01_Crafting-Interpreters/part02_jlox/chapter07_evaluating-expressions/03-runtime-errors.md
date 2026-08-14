# 第 7 章 · Evaluating Expressions（求值表达式） · §7.3 运行时错误（Runtime Errors）

← [本章目录](./README.md) · 上一节：[02-evaluating-expressions.md](./02-evaluating-expressions.md) · 下一节：[04-hooking-up-the-interpreter.md](./04-hooking-up-the-interpreter.md)

---

- 非法运算（如 `-"muffin"`）→ 不应让 Java **`ClassCastException`** 崩掉整个 VM。
- **类型检查**后再运算；失败 → 抛自定义 **`RuntimeError`**（带**行号**）。

| 阶段 | 错误类型 |
|------|----------|
| 词法 | Scanner / ch4 |
| 语法 | `ParseError` / ch6 |
| **运行时** | **`RuntimeError`** / 本章 |

---
