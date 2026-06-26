# Ch 3 指令级并行及其开发 · Instruction-Level Parallelism and Its Exploitation

> **Computer Architecture 6th** · Hennessy & Patterson · **选读 🟡**

> 本章定位：**单核如何「一周期多条指令」** — 从数据/控制相关性、分支预测、动态调度、硬件推测到多发射与 SMT。HFT 热路径若 ILP 吃不满，瓶颈往往在 **分支与 cache miss**，而非「再抠几条算术指令」。

**核心问题：** 两条指令能否并行？硬件如何用 **乱序执行 + 推测 + 顺序提交** 在维持正确性的前提下把 CPI 压到 1 以下？

```
相关性 → 冒险 (RAW/WAR/WAW) → 停顿 or 旁路/重命名/推测
编译器展开/调度 + 分支预测 + Tomasulo/ROB + 多发射 → ILP
ILP 见底 → 多线程/SMT（→ 3.11，衔接 Ch5 TLP）
```

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 3.1 ILP 的概念与挑战 | [notes/section-3.1-ILP概念与挑战.md](./notes/section-3.1-ILP概念与挑战.md) |
| 3.2 编译器揭示 ILP | [notes/section-3.2-编译器揭示ILP.md](./notes/section-3.2-编译器揭示ILP.md) |
| 3.3 高级分支预测 | [notes/section-3.3-高级分支预测.md](./notes/section-3.3-高级分支预测.md) |
| 3.4–3.5 动态调度与 Tomasulo | [notes/section-3.4-3.5-动态调度与Tomasulo.md](./notes/section-3.4-3.5-动态调度与Tomasulo.md) |
| 3.6 硬件推测与 ROB | [notes/section-3.6-硬件推测与ROB.md](./notes/section-3.6-硬件推测与ROB.md) |
| 3.7–3.9 多发射与指令交付 | [notes/section-3.7-3.9-多发射与指令交付.md](./notes/section-3.7-3.9-多发射与指令交付.md) |
| 3.11 多线程技术 | [notes/section-3.11-多线程技术.md](./notes/section-3.11-多线程技术.md) |
| 3.12 实例：Cortex-A53 与 Core i7 | [notes/section-3.12-实例分析-Cortex-A53与Core-i7.md](./notes/section-3.12-实例分析-Cortex-A53与Core-i7.md) |

---

## HFT 精读捷径

| 本节 | 带走什么 |
|------|----------|
| 3.1 | **RAW + 控制相关** — 热路径依赖链与不可预测分支 = ILP 杀手 |
| 3.2–3.3 | **循环展开、少分支、可预测分支** — 比手写微优化更值钱 |
| 3.4–3.6 | 理解 **乱序+推测** — 为何 cache miss 时 IPC 崩塌 |
| 3.7–3.9 | **CPI < 1** 需多发射；BTB 对取指带宽 — 深流水线换频率的代价 |
| 3.11 | **SMT/超线程** — 实盘常 **关 HT、一核一线程** 换确定性 |
| 3.12 | A53 **顺序双发射** vs i7 **乱序+SMT** — 与 Ch2 实例对照读 |

→ [Ch2 存储器层次](../chapter-02-memory-hierarchy-design/) · [01-CSAPP Ch4](../../01-CSAPP-3rd/chapter-04-processor-architecture/) · [Ch5 TLP](../chapter-05-线程级并行.md)

---

## 相关章节

- 上一章：[chapter-02-memory-hierarchy-design](../chapter-02-memory-hierarchy-design/)
- 下一章：[chapter-04-vector-simd-gpu](../chapter-04-vector-simd-gpu/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
