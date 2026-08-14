# 第 3 章 · 语法分析 · 本章定位

← [本章目录](./README.md) · 上一章：[ch2 扫描](../chapter02_scanners/README.md) · 下一节：[01-cfg-bnf-and-parse-trees.md](./01-cfg-bnf-and-parse-trees.md)

---

| 章 | 角色 |
|:--:|------|
| **ch2** | Token 流 — 「单词」 |
| **ch3** | 语法结构 — 「句子」 |

```text
Token stream  →  Parser (ch3)  →  Parse tree / AST  →  语义分析 (ch4+)
```

*Crafting Interpreters*：**jlox** 手写递归下降 + AST；**clox** 用 **Pratt 解析** 边解析边发字节码（仍属自顶向下家族）。橡书 ch3 补全 **LR/YACC** 理论与 **文法改造** 数学基础。
