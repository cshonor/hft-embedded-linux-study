# 第 7 章 · Evaluating Expressions（求值表达式） · §7.1 表示值（Representing Values）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-evaluating-expressions.md](./02-evaluating-expressions.md)

---

**问题**：Lox **动态类型** vs Java **静态类型**。

**方案**：用 Java **`Object`** 存任意 Lox 运行时值：

| Lox | Java 表示 |
|-----|-----------|
| `nil` | `null` |
| Number | `Double` |
| Boolean | `Boolean` |
| String | `String` |

后续 ch8 变量、ch10 函数等仍在此模型上扩展。

---
