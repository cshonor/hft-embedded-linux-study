# 第 3 章 · The Lox Language（Lox 语言概览） · §3.4 表达式（Expressions）

← [本章目录](./README.md) · 上一节：[03-data-types.md](./03-data-types.md) · 下一节：[05-statements.md](./05-statements.md)

---

| 类别 | 要点 |
|------|------|
| 算术 / 比较 | 常规中缀表达式 |
| 逻辑 | `!`、`and`、`or` |
| **短路求值** | `and` / `or` 具 **short-circuit** → 更像「披着表达式外衣的控制流」 |
| 分组 | `()` 改变运算符优先级 |

**实现预告**：Part II **ch6～7** 解析与求值表达式；clox **ch17** 编译表达式到字节码。

**Rust 对照**：`&&` / `||` 同样短路；但 Rust 中它们是**控制流**而非可嵌在任意表达式位置的「值」（与 Lox 略有不同）。

---
