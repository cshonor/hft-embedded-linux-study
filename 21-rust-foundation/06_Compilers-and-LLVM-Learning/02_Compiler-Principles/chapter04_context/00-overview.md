# 第 4 章 · 上下文相关分析 · 本章定位

← [本章目录](./README.md) · 上一章：[ch3 语法分析](../chapter03_parsers/README.md) · 下一节：[01-why-context-sensitive.md](./01-why-context-sensitive.md)

---

| 阶段 | 能回答什么 |
|------|------------|
| **ch2 扫描** | 单词是否合法 |
| **ch3 语法** | 句子结构是否合法 |
| **ch4 上下文相关** | **在上下文中是否有意义** |

```text
Parse tree / 语法结构
    →  Context-sensitive analysis (ch4)
    →  AST + 符号表 + 类型信息
    →  IR (ch5+)
```

很多文献将本章内容称为 **语义分析（Semantic Analysis）** — 与 ch3「语法分析」前后衔接，构成前端理解程序的完整闭环。

**Part I 前端**（ch2～4）在此章收尾；**Part II 基础结构**（ch5 IR…）从 ch5 开始。
