# 第 2 章 · 扫描 · 本章定位

← [本章目录](./README.md) · 上一章：[ch1 编译总览](../chapter01_overview/README.md) · 下一节：[01-scanner-core-tasks.md](./01-scanner-core-tasks.md)

---

| 章 | 角色 |
|:--:|------|
| **ch1** | 编译器**宏观总览** · 三阶段 · Trade-offs |
| **ch2** | 前端**第一步**：字符流 → **Token 流** |

```text
Source characters  →  Scanner (ch2)  →  Token stream  →  Parser (ch3)
```

本章重点：**理论**（RE、NFA、DFA、Thompson / 子集构造 / Hopcroft）+ **工程**（表驱动 vs 直接编码、关键字散列）。

*Crafting Interpreters* 中 jlox/clox 是**手写扫描器**入门；橡书 ch2 补全「为何 lex/flex 那套能自动生成」的数学底座。
