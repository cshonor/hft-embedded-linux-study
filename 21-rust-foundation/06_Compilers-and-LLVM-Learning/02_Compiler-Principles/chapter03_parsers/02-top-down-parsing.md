# 第 3 章 · 语法分析 · §2 自顶向下分析（Top-Down）

← [本章目录](./README.md) · 上一节：[01-cfg-bnf-and-parse-trees.md](./01-cfg-bnf-and-parse-trees.md) · 下一节：[03-bottom-up-parsing.md](./03-bottom-up-parsing.md)

---

从分析树**根（开始符号）**出发，**自上而下**扩展，直到匹配输入**叶子（Token）**。

---

## 难点与文法改造

| 问题 | 后果 | 改造 |
|------|------|------|
| **左递归** | `A → A α \| β` 导致自顶向下**死循环** | **消除左递归** |
| **公共左因子** | 多条产生式同前缀 → **回溯**低效 | **提取左因子** |

改造后文法才适合 predictive / LL 分析。

---

## 递归下降分析器（Recursive Descent）

| 特点 | 说明 |
|------|------|
| **工程最爱** | 每个非终结符 ≈ **一个函数** |
| **Lookahead** | 预看 1 个 Token 决定走哪条产生式 |
| **对应文法类** | **LL(1)** — 可手工编写、逻辑清晰 |

```text
parseExpression()  →  parseTerm()  →  parseFactor()
         ↑ 调用链即分析树自顶向下展开
```

**CI jlox** 即此路线 → [ch6 Parsing Expressions](../../../01_Crafting-Interpreters/part02_jlox/chapter06_parsing-expressions/README.md)

**clox Pratt** — 自顶向下的运算符优先级解析变体 → [ch17 § Pratt](../../../01_Crafting-Interpreters/part03_clox/chapter17_compiling-expressions/02-a-pratt-parser.md)

---

## LL(1) 直觉

- **L** — 从左到右读输入（Left-to-right）
- **L** — 最左推导（Leftmost derivation）
- **(1)** — 向前看 **1** 个 Token 即可决定

| 优点 | 缺点 |
|------|------|
| 易手写、易调试、错误信息可控 | 文法类**较窄**；左递归须先改文法 |

---

## 与自底向上对比（预告）

LL 手工友好；工业 C/Java 级文法很多更适合 **LR** → [§3](./03-bottom-up-parsing.md)
