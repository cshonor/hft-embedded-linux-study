# 第 12 章 · 指令调度（Instruction Scheduling）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part IV 代码生成

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**后端第二关** — ch11 选出指令后，在**不改语义**前提下**重排顺序**以隐藏 **stall**（load/FP 延迟）；核心武器 **列表调度**（依赖图 + 就绪队列 + 优先级）；**NP 完全**故用启发式；与 ch13 **寄存器分配**存在 **phase ordering** 冲突；进阶 **区域调度 · 上下文复制**。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 核心问题与动机 | [01-motivation-and-problem.md](./01-motivation-and-problem.md) |
| §2 | 列表调度 | [02-list-scheduling.md](./02-list-scheduling.md) |
| §3 | 调度与寄存器分配的矛盾 | [03-scheduling-vs-regalloc.md](./03-scheduling-vs-regalloc.md) |
| §4 | 高级话题 | [04-advanced-topics.md](./04-advanced-topics.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch12 | 对照 |
|----------|------|
| ch1 后端三难点 | [ch1 §4.3](../chapter01_overview/04-translation-pipeline-example.md) |
| ch11 指令筛选 | [ch11](../chapter11_instr-select/README.md) — 调度在其后 |
| ch9 活跃变量 | [ch9 §1](../chapter09_dataflow/01-iterative-dataflow.md) — 调度增 LIVE → spill |
| ch13 寄存器分配 | 下一章 — phase ordering 核心 |
| HFT 延迟 | 热点路径 stall / spill 极敏感 |
| LLVM | `MachineScheduler` · post-RA / pre-RA 调度 |

---

## 逻辑脉络

动机与 NP 性 → 列表调度 → 与 regalloc 博弈 → 区域扩展。

---

## 速记

## 本章速记

```text
§1  stall / RAW · 隐藏延迟 · NP 完全 → 启发式
§2  列表调度：Ready 队列 + 优先级 · 前向/向后
§3  调度 ↔ regalloc：LIVE↑ · spill 可抵消收益
§4  区域调度 · 上下文复制 · 软件流水线（概念）
```

---

## 三句背诵

1. **调度 = 不改语义地重排机器指令，填 stall 槽。**
2. **列表调度：依赖 DAG + Ready + 贪心优先级。**
3. **调度与寄存器分配互相牵制 — phase ordering 无银弹。**

---

## 对照表

| 概念 | 一句话 |
|------|--------|
| Stall | 等前序结果，流水线空转 |
| 关键路径优先 | 最长延迟链上的指令先发 |
| Spill | 寄存器不够，溢栈 ↔ 高延迟 |
| Regional | 跨基本块调度 |

---

## 自测

- [ ] RAW 与 stall 的关系
- [ ] 列表调度四步概览
- [ ] 调度为何增加寄存器压力
- [ ] NP 完全性对工程意味着什么

---

## 阅读进度

- [x] ch12 指令调度
- [x] ch13 寄存器分配

