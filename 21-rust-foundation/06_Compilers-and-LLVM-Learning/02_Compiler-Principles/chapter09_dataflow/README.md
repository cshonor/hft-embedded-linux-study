# 第 9 章 · 数据流分析（Data-Flow Analysis）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part III 优化

## 状态

- [x] 已读（笔记整理）

---

## 一句话

优化阶段的**分析引擎**：**迭代数据流**（以**活跃变量 / LIVEOUT** 为例）在 CFG 上传播程序状态；**SSA 构建**（支配 · φ 放置 · 重命名）与 **SSA 销毁**；进阶 **可约 CFG** 与 **过程间分析** — 为 ch10 标量 Pass 提供理论与工具。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 迭代数据流分析 · 活跃变量 | [01-iterative-dataflow.md](./01-iterative-dataflow.md) |
| §2 | 支配与 φ 放置 | [02-dominance-and-phi.md](./02-dominance-and-phi.md) |
| §3 | SSA 重命名与代码复原 | [03-ssa-renaming-and-destruction.md](./03-ssa-renaming-and-destruction.md) |
| §4 | 高级话题 | [04-advanced-topics.md](./04-advanced-topics.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch9 | 对照 |
|----------|------|
| ch8 可用表达式 | [ch8 §4](../chapter08_intro-opt/04-global-redundancy.md) — 同类数据流 |
| ch5 SSA 概念 | [ch5 §5](../chapter05_ir/05-ssa-form.md) — 本章讲**怎么建/拆** |
| LLVM SSA | [04 LLVM IR](../../../04_Learn-LLVM-17/README.md) |
| ch10 标量优化 | 下一章 — 在 SSA 上跑 Pass |

---

## 逻辑脉络

迭代框架 + 实例 → 建 SSA → 拆 SSA → 进阶理论。

---

## 速记

## 本章速记

```text
§1  迭代数据流 IN/OUT · 活跃变量 LIVEOUT（反向）· 不动点
§2  支配/idom/支配边界 · 最小 φ 放置
§3  支配树重命名 · SSA 销毁=φ→mov · 并行拷贝
§4  可约CFG · 过程间分析 · LTO/IPO
```

---

## 三句背诵

1. **数据流 = CFG 上方程迭代到不动点。**
2. **建 SSA：支配边界插 φ，支配树重命名。**
3. **优化完拆 SSA，再交给代码生成/regalloc。**

---

## 分析对照表

| 分析 | 方向 | 典型用途 |
|------|------|----------|
| 可用表达式 | 正向 | CSE |
| 活跃变量 | **反向** | regalloc、DCE |
| 到达定值 | 正向 | 常量/拷贝传播 |

---

## 自测

- [ ] LIVEOUT 方程各一项含义
- [ ] 支配与支配边界各一句话
- [ ] SSA 构建三步：φ、rename、（优化）、destroy
- [ ] 过程间分析为何贵

---

## 阅读进度

- [x] ch9 数据流分析
- [x] ch10 标量优化

