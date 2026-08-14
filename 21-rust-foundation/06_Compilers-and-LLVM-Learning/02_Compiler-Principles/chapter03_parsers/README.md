# 第 3 章 · 语法分析（Parsers）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part I 前端

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch2 识别**单词（Token）**；**ch3 识别句子** — 验证 Token 排列是否符合语法，并构建**层次结构**（分析树 / AST）。两大路线：**自顶向下（LL / 递归下降）** vs **自底向上（LR / 移入-归约）**；工具链侧 **YACC/Bison** 背后是 **LR(1) Action/Goto 表**。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | CFG · BNF · 分析树 · 歧义 | [01-cfg-bnf-and-parse-trees.md](./01-cfg-bnf-and-parse-trees.md) |
| §2 | 自顶向下分析（LL / 递归下降） | [02-top-down-parsing.md](./02-top-down-parsing.md) |
| §3 | 自底向上分析（移入-归约 / LR） | [03-bottom-up-parsing.md](./03-bottom-up-parsing.md) |
| §4 | LR(1) 自动生成（项目集 · Action/Goto） | [04-lr-automation.md](./04-lr-automation.md) |
| §5 | 工程实践（错误恢复 · 表压缩 · 特例） | [05-engineering-practice.md](./05-engineering-practice.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch3 | 对照 |
|----------|------|
| 递归下降 + AST | [CI jlox ch5～6](../../../01_Crafting-Interpreters/part02_jlox/chapter06_parsing-expressions/README.md) |
| Pratt 解析 | [CI clox ch17 Pratt parser](../../../01_Crafting-Interpreters/part03_clox/chapter17_compiling-expressions/02-a-pratt-parser.md) |
| CFG 入门 | [CI jlox ch5 CFG](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/01-context-free-grammars.md) |
| ch2 Token 输入 | [ch2 扫描](../chapter02_scanners/README.md) |

---

## 逻辑脉络

CFG 描述语法 → 消歧/改造文法 → 自顶向下 **或** 自底向上 → LR 表驱动 → 错误恢复与工程折中。

---

## 速记

## 本章速记

```text
§1  CFG/BNF · 派生/分析树 · 歧义(悬挂else/优先级)→重写文法
§2  自顶向下 · 消左递归/提左因子 · 递归下降=LL(1) · CI jlox
§3  自底向上 · shift-reduce · 句柄 · LR(1) · yacc/bison
§4  项目集闭包+Goto → DFA → Action/Goto 表
§5  错误恢复 · LALR/SLR 表压缩 · 一元符/列表/上下文补 ch4
```

---

## 三句背诵

1. **ch2 单词，ch3 句子：CFG 描述，分析树/AST 记录结构。**
2. **手写走 LL 递归下降；工具走 LR 移入-归约 + Action/Goto 表。**
3. **歧义改文法；LR(1) 太大就 LALR 合并状态。**

---

## 与 CI 对照

| 橡书 ch3 | CI |
|----------|-----|
| CFG / 歧义 | [jlox ch5 CFG](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/01-context-free-grammars.md) |
| 递归下降 | [jlox ch6](../../../01_Crafting-Interpreters/part02_jlox/chapter06_parsing-expressions/README.md) |
| 运算符解析 | [clox Pratt ch17](../../../01_Crafting-Interpreters/part03_clox/chapter17_compiling-expressions/02-a-pratt-parser.md) |

---

## 自测

- [ ] CFG 四要素 · BNF 是什么
- [ ] 歧义举例 + 一种消除法
- [ ] 左递归为何害自顶向下 · 两种文法改造
- [ ] shift vs reduce 各一句话
- [ ] Action 表 vs Goto 表
- [ ] LALR 相对 LR(1) 折中了什么

---

## 阅读进度

- [x] ch3 语法分析（本章笔记）
- [ ] ch4 上下文相关分析

