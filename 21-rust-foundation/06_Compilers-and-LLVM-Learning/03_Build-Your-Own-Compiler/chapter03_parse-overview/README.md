# 第 3 章 · 语法分析的概要

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第1部分 · 代码分析

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**第1部分开篇** — 代码分析三阶段（**词法 / 语法 / 语义**）；**Scanner → token**（字面+种类+语义值）；**Parser → AST**（节点、省略纯格式符号）；**解析器生成器**（LL vs LALR → 选 **JavaCC**）；**.jj** 文件结构与 `javacc` 工作流。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 语法分析的方法 | [01-analysis-phases-and-tokens.md](./01-analysis-phases-and-tokens.md) |
| §2 | 解析器生成器 | [02-parser-generators.md](./02-parser-generators.md) |
| §3 | JavaCC 的概要 | [03-javacc-overview.md](./03-javacc-overview.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch3 | 对照 |
|----------|------|
| ch2 cbc | [chapter02_cflat-cbc](../chapter02_cflat-cbc/README.md) · `parser` 包 |
| ch4～6 | 词法 · JavaCC 规则 · 完整 parser |
| CI 扫描/语法 | [CI ch4 扫描](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/) · [ch5 文法](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/) |
| EaC | [ch2 扫描](../../../02_Compiler-Principles/chapter02_scanners/) · [ch3 语法分析](../../../02_Compiler-Principles/chapter03_parsers/) |

---

## 逻辑脉络

三阶段与 token/AST → 为何用生成器 → JavaCC 入门格式。

---

## 速记

## 本章速记

```text
§1  词法→语法→语义 · token(字面+种类+值) · Parser→AST/节点
§2  生成器 · LALR(yacc) vs LL(JavaCC) · 可读/无额外库
§3  .jj · javacc→.java · UNICODE_INPUT + UTF-8 Reader
```

---

## 三句背诵

1. **Scanner 出 token，Parser 出 AST。**
2. **本书用 JavaCC（LL），不用 yacc。**
3. **`.jj` → javacc → javac → 进 cbc。**

---

## 对照表

| 术语 | 一句话 |
|------|--------|
| Scanner | 切词 + 分类 + 语义值 |
| Token | 单词三要素打包 |
| AST | 省略纯格式符号的结构树 |
| JavaCC | Java 用 LL 解析器生成器 |

---

## 自测

- [ ] 分析三阶段顺序与产物
- [ ] token 与 AST 节点各是什么
- [ ] LL 选 JavaCC 的原因（说两条）
- [ ] `.jj` 文件主要组成部分

---

## 阅读进度

- [x] ch3 语法分析的概要
- [x] ch4 词法分析

