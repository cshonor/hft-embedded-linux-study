# 第 16 章 · Scanning on Demand（按需扫描） · §16.4 标识符与关键字（Identifiers and Keywords）

← [本章目录](./README.md) · 上一节：[02-lexemes-and-whitespace.md](./02-lexemes-and-whitespace.md) · 下一节：[04-jlox-vs-clox.md](./04-jlox-vs-clox.md)

---

标识符：`[a-zA-Z_][a-zA-Z0-9_]*` → 先扫完整 lexeme。

**关键字识别**：C 无内置 HashMap → 书中用 **确定性有限自动机 (DFA)** / **Trie 路径** = **嵌套 switch**。

```text
首字母 'f':
  后续 "alse"  → TOKEN_FALSE
  后续 "un"    → TOKEN_FUN
  否则         → TOKEN_IDENTIFIER
```

| 特点 | 说明 |
|------|------|
| **无堆分配** | 不建关键字表 |
| **分支预测友好** | 常见字快速路径 |
| **最长匹配** | 整词比对，避免 `for` vs `foreign` 误判 |

**若不是关键字** → **`TOKEN_IDENTIFIER`**（变量名、函数名等）。

**Lox 保留字**（与 ch3 规格一致）：`and class else false fun for if nil or print return super this true var while` 等。

---
