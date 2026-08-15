# 第 2 章 · 扫描（Scanners / 词法分析）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part I 前端

## 状态

- [x] 已读（笔记整理）

---

## 一句话

第 1 章是宏观总览；**第 2 章正式踏入前端第一站** — 从源代码**字符流**中提取符合语言规则的「单词」（**Token**），并确定词性。核心链路：**正则表达式 → NFA → DFA → 最小化 DFA → 表驱动/直接编码实现**，辅以**散列表关键字识别**等工程权衡。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 扫描器的核心任务 | [01-scanner-core-tasks.md](./01-scanner-core-tasks.md) |
| §2 | 正则表达式（RE） | [02-regular-expressions.md](./02-regular-expressions.md) |
| §3 | 有穷自动机（NFA / DFA） | [03-finite-automata.md](./03-finite-automata.md) |
| §4 | 自动生成管线 | [04-re-to-dfa-pipeline.md](./04-re-to-dfa-pipeline.md) |
| §5 | 工程实现 | [05-scanner-implementation.md](./05-scanner-implementation.md) |
| §6 | 关键字识别与权衡 | [06-keywords-and-tradeoffs.md](./06-keywords-and-tradeoffs.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch2 | 对照 |
|----------|------|
| 手写扫描器 | [CI jlox ch4 Scanning](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/README.md) |
| 按需扫描 | [CI clox ch16 Scanning on Demand](../../../01_Crafting-Interpreters/part03_clox/chapter16_scanning-on-demand/README.md) |
| 流水线位置 | [CI ch2 · 上山前端](../../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) |
| ch1 三阶段 | [ch1 §3 前端](../chapter01_overview/03-three-phase-structure.md) |

---

## 逻辑脉络

RE 描述「找什么」→ FA 描述「怎么找」→ 构造算法 → 代码形态 → 关键字工程技巧。

---

## 速记

## 本章速记

```text
§1  字符流→Token流 · 过滤空白/注释 · 前端第一站
§2  RE：选择|连接|克林闭包* · 描述微语法
§3  NFA（多路径/ε）vs DFA（唯一转移）· 扫描器用 DFA
§4  Thompson → 子集构造 → Hopcroft 最小化
§5  表驱动（查表）vs 直接编码（switch/goto，常更快）
§6  关键字：先当标识符 · 散列表改词性 · 简化 DFA
```

---

## 三句背诵

1. **RE 说找什么，DFA 说怎么找；扫描器跑最小化 DFA。**
2. **Thompson → 子集构造 → Hopcroft，是 lex 类工具的理论链。**
3. **关键字别硬塞进 DFA，标识符 + 哈希表更工程。**

---

## 与 CI 对照

| 橡书 ch2 | CI |
|----------|-----|
| RE→DFA 理论 | jlox/clox **手写** Scanner |
| Token 概念 | [ch4 Scanning](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/README.md) · [ch16](../../../01_Crafting-Interpreters/part03_clox/chapter16_scanning-on-demand/README.md) |

---

## 自测

- [ ] 扫描器输入/输出各是什么？
- [ ] RE 三基本操作是什么？
- [ ] NFA 与 DFA 各一条区别
- [ ] 子集构造、Hopcroft 各解决什么问题
- [ ] 表驱动 vs 直接编码如何权衡
- [ ] 为何关键字用散列表而不是扩 DFA

---

## 阅读进度

- [x] ch2 扫描（本章笔记）
- [ ] ch3 语法分析

