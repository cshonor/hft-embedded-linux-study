# 第 8 章 · 代码优化概述（Introduction to Code Optimization）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part III 优化

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**Part III 优化开篇** — 用 LINPACK 等实例建立宏观视角：优化按**作用域**分层（局部→全过程）；核心工具 **值编号** 与 **DAG** 消冗余；再扩展到超局部/支配者、**可用表达式**全局消除、**内联**等过程间手段 — 为 ch9 数据流、ch10 标量优化铺垫。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 优化的作用域 | [01-scope-of-optimization.md](./01-scope-of-optimization.md) |
| §2 | 消除冗余表达式（DAG · 值编号） | [02-redundant-expressions.md](./02-redundant-expressions.md) |
| §3 | 扩展到更大区域 | [03-beyond-basic-blocks.md](./03-beyond-basic-blocks.md) |
| §4 | 全局冗余消除 | [04-global-redundancy.md](./04-global-redundancy.md) |
| §5 | 过程间优化（内联等） | [05-interprocedural-opt.md](./05-interprocedural-opt.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch8 | 对照 |
|----------|------|
| 基本块 / CFG | [ch5 §3 CFG](../chapter05_ir/03-graphical-ir.md) |
| 代码形态 | [ch7](../chapter07_code-shape/README.md) — 形态决定可优化性 |
| ch1 实用性 | [ch1 §2 实用性](../chapter01_overview/02-two-fundamental-principles.md) |
| LLVM O0/O3 | [04 optimize_compare](../../../04_Learn-LLVM-17/ir_samples/optimize_compare/) |
| CI 局部优化 | [clox ch30](../../../01_Crafting-Interpreters/part03_clox/chapter30_optimization/README.md) |

---

## 逻辑脉络

作用域框架 → 块内 DAG/VN → 跨块 VN/支配者 → 可用表达式 → 内联 — 然后 ch9 系统化数据流。

---

## 速记

## 本章速记

```text
§1  Local→Superlocal→Regional→Global→Whole-program · 代价↑机会↑
§2  块内 DAG · 值编号 VN · 消冗余
§3  超局部 VN · 支配者 VN · 跨块
§4  可用表达式 · 全局 CSE · ch9 数据流
§5  内联 · 复制/特化 · LTO · Part III 开篇
```

---

## 三句背诵

1. **优化按作用域分层；块内 VN，全局靠数据流。**
2. **可用表达式：问「这式子前面算不算过且操作数没变」。**
3. **内联换体积，换上下文，换更多全局优化机会。**

---

## 与仓库对照

| 橡书 ch8 | 本仓库 |
|----------|--------|
| 可用表达式/ch9 | 04 LLVM Pass |
| VN/GVN | O0 vs O3 IR diff |
| 局部优化直觉 | clox ch30 |

---

## 自测

- [ ] 五层作用域各一句
- [ ] DAG 与 VN 各解决什么
- [ ] 可用表达式分析解决什么问题
- [ ] 内联的收益与代价

---

## 阅读进度

- [x] ch8 优化概述
- [ ] ch9 数据流分析

