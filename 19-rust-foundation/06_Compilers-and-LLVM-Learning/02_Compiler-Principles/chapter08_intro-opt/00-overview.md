# 第 8 章 · 代码优化概述 · 本章定位

← [本章目录](./README.md) · 上一章：[ch7 代码形态](../chapter07_code-shape/README.md) · 下一节：[01-scope-of-optimization.md](./01-scope-of-optimization.md)

---

```text
Part I  前端 (ch2～4)     → 理解程序
Part II 基础结构 (ch5～7)  → IR + 运行时 + lowering 形态
Part III 优化 (ch8～10)   → 改进 IR（不改语义）
Part IV 代码生成 (ch11～13)
```

| ch1 三阶段 | Part III |
|------------|----------|
| **Optimizer** | ch8 概念框架 · ch9 数据流引擎 · ch10 SSA 标量 Pass |

本章**不**先堆公式 — 用 **LINPACK** 等实例 + **作用域** + **值编号** 搭优化器地图。

**前提**： [ch5 SSA](../chapter05_ir/05-ssa-form.md) · [ch7 形态](../chapter07_code-shape/README.md) 越好，优化越有效。
