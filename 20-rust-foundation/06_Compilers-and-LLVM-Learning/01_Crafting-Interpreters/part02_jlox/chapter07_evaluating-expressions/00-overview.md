# 第 7 章 · Evaluating Expressions（求值表达式） · 本章定位

← [本章目录](./README.md) · 下一节：[01-representing-values.md](./01-representing-values.md)

---

解释器**「睁开眼睛」**：从静态 AST → **真正执行并算出结果**。

| ch6 | ch7 | ch8 |
|-----|-----|-----|
| Token → AST | **AST → 值** | 语句 + 变量 + 状态 |

| 小节 | 主题 |
|------|------|
| **§7.1** | Lox 值 ↔ Java `Object` 映射 |
| **§7.2** | `Interpreter` 实现 `Expr.Visitor` · 后序遍历 |
| **§7.3** | 运行时类型错误 · `RuntimeError` |
| **§7.4** | `interpret()` · `stringify()` · REPL 不退出 |

---
