# 第 1 章 · Welcome（Introduction） · §1.2 本书是如何组织的（How the Book is Organized）

← [本章目录](./README.md) · 上一节：[01-why-learn-this-stuff.md](./01-why-learn-this-stuff.md) · 下一节：[03-the-first-interpreter.md](./03-the-first-interpreter.md)

---

### 三 Part +「一章一特性」

全书分 **三个部分**；每个**实现章**通常只聚焦**一个语言特性**：先讲概念，再写代码。

| Part | 章次 | 实现 | 语言 |
|------|------|------|------|
| **I · Welcome** | 1～3 | — | 术语 + Lox 规格 |
| **II · Tree-Walk** | 4～13 | **jlox** | Java |
| **III · Bytecode VM** | 14～30 | **clox** | C |

### 四个特色板块

| 板块 | 英文 | 作用 | 阅读建议 |
|------|------|------|----------|
| **代码** | The code | **每一行**真实实现代码，无遗漏 | 正文必读 |
| **旁注** | Asides | 历史、人物、趣闻 | 可跳过，不影响跟代码 |
| **挑战** | Challenges | 章末改/扩解释器 | 时间允许则做，吸收最扎实 |
| **设计笔记** | Design notes | **如何设计**语言（易学、可读、命名…） | 实现书少见；值得细读 |

**纯手工**：不用 Lex / Yacc 等生成器，避免「中间层看不见」的死角（与 §1.1 去魅一致）。

---
