# 第 5 章 · 基于 JavaCC 的解析器的描述

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第1部分 · 代码分析

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**Parser 规则篇** — 扫描器只产 **token**；**Parser** 用 **EBNF** 写 **非终端符**（`stmt()`/`expr()`）识别语句/表达式并建树；**连接 · * · + · | · []**；解决 **二义性/选择冲突**：提取公因子 · **`LOOKAHEAD(n)`** · 规则形式 LOOKAHEAD（含 **dangling else**）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 基于 EBNF 语法的描述 | [01-ebnf-grammar.md](./01-ebnf-grammar.md) |
| §2 | 语法的二义性与 token 超前扫描 | [02-ambiguity-and-lookahead.md](./02-ambiguity-and-lookahead.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch5 | 对照 |
|----------|------|
| ch4 词法 | [chapter04_lexical](../chapter04_lexical/README.md) · TOKEN 即终端符 |
| ch6 下一章 | 完整 C♭ 定义/语句/表达式文法 |
| CI | [ch5 上下文无关文法](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/01-context-free-grammars.md) |
| EaC | [ch3 语法分析](../../../02_Compiler-Principles/chapter03_parsers/) |

---

## 逻辑脉络

EBNF 与非终端符 → 二义性 → LOOKAHEAD 消冲突。

---

## 速记

## 本章速记

```text
§1  Parser：token→语句/表达式/树 · 终端 vs 非终端 · EBNF * + | []
§2  二义性 · Choice conflict · 提公因子 · LOOKAHEAD(n/规则) · else
```

---

## 三句背诵

1. **终端 = token/字面；非终端 = stmt()/expr() 规则。**
2. **`|` 同首 → 选择冲突；默认只看 1 token。**
3. **LOOKAHEAD 消冲突；dangling else 用 LOOKAHEAD(1) 绑内层 if。**

---

## EBNF 对照

| 符号 | 含义 |
|------|------|
| 并排 | 连接 |
| `*` | 0+ |
| `+` | 1+ |
| `\|` | 选择 |
| `[]` | 可选 |

---

## 自测

- [ ] Parser 相对 Scanner 多做什么
- [ ] 写一条含 `|` 和 `[]` 的概念产生式
- [ ] 解释 dangling else 与 LOOKAHEAD(1) 的关系
- [ ] LOOKAHEAD 填数字 vs 填规则

---

## 阅读进度

- [x] ch5 基于 JavaCC 的解析器的描述
- [x] ch6 语法分析

