# 第 5 章 · 中间表示 · 本章定位

← [本章目录](./README.md) · 上一章：[ch4 上下文相关分析](../chapter04_context/README.md) · 下一节：[01-why-ir.md](./01-why-ir.md)

---

**Part II 基础结构** 开篇：前端已产出 AST/类型/符号信息；**ch5 起讨论编译器内部如何表示程序本体**。

```text
Source  →  Front end (ch2～4)  →  IR (ch5)  →  Opt (ch8～10)  →  Back end (ch11～13)
```

| 概念 | 本章 |
|------|------|
| **IR** | 连接前端与后端的**通用程序表示** |
| **代码形态** | 选 AST / CFG / 三地址 / SSA… 影响**可做的优化** |

与 [ch1 三阶段](../chapter01_overview/03-three-phase-structure.md) 中 **Optimizer 的输入** 直接对应。
