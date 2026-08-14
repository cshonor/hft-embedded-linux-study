# 第 6 章 · 语法分析 · 本章定位

← [本章目录](./README.md) · 上一章：[ch6 语法分析](../chapter06_parsing/README.md) · 下一章：[ch7 JavaCC 的 action 和抽象语法树](../chapter07_javacc-ast/README.md) · 下一节：[01-definitions.md](./01-definitions.md)

---

```text
第1部分
  ch3  概念
  ch4  TOKEN
  ch5  EBNF + LOOKAHEAD
  ch6  完整 C♭ 文法  ← 本章 · 第1部分收官
  ↓
第2部分 ch7  解析动作 → AST
```

| [ch2 `compile`](../chapter02_cflat-cbc/03-compiler-control-flow.md) | 本章 |
|-----------------------------------------------------------------------|------|
| parse → AST | **`.jj` 里写齐** 定义/语句/表达式/项 |

**自上而下阅读顺序** = 程序结构 → 语句 → 表达式 → 原子项 — 与 JavaCC 规则分层一致。
