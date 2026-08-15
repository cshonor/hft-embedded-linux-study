# 第 9 章 · Control Flow（控制流） · §9.3 逻辑操作符（Logical Operators）

← [本章目录](./README.md) · 上一节：[01-if-statements.md](./01-if-statements.md) · 下一节：[03-while-loops.md](./03-while-loops.md)

---

`and` / `or` 在 Lox 中：

| 特性 | 说明 |
|------|------|
| **控制流味道** | 不仅是算术式二元运算 |
| **短路（Short-circuit）** | 左操作数已能定结果 → **不求值**右操作数 |
| **返回值** | 返回**决定结果的那个操作数的原值**，不一定是 `true/false` |

示例：

```lox
"hi" or 2    // → "hi"（左 truthy，右不执行）
nil and foo  // → nil（左 falsy，右不执行）
```

**实现位置**：

- **Parser**：`and` / `or` 优先级低于 equality（ch6 分层已预留）或单独逻辑层。
- **Interpreter**：`visitLogicalExpr` 中按短路规则求值，**不**一律返回 Boolean。

**对照 ch3 §3.4**：表达式层的短路 · 与 C/Java `&&`/`||` 返回 boolean 的差异。

---
