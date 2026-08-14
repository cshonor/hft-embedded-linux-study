# 第 5 章 · 基于 JavaCC 的解析器的描述 · 本章定位

← [本章目录](./README.md) · 上一章：[ch5 基于 JavaCC 的解析器的描述](../chapter05_javacc-parser/README.md) · 下一章：[ch6 语法分析](../chapter06_parsing/README.md) · 下一节：[01-ebnf-grammar.md](./01-ebnf-grammar.md)

---

```text
ch4  .jj 词法（TOKEN / SKIP / 状态）
  ↓
ch5  .jj 语法（EBNF + LOOKAHEAD）  ← 本章
  ↓
ch6  C♭ 完整 parser
```

| 层次 | 负责 |
|------|------|
| Scanner | 单个 **token** |
| **Parser** | token 序列 → **语句 / 表达式 / …** → **语法树** |

**同一 `.jj` 文件** — ch4 与 ch5 规则并存；`javacc` 一次生成 Scanner+Parser。
