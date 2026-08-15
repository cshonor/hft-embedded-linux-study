# 第 13 章 · 寄存器分配（Register Allocation）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part IV 代码生成

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**后端第三关 · Part IV 收官** — 无限**虚拟寄存器** → 有限**物理寄存器**：**分配 vs 赋值** · 局部策略 → **存活范围** → 全局 **图着色**（冲突图 · spill 代价 · **接合 coalescing**）；编码机器约束与工程难题 — 与 ch12 **phase ordering** 呼应，完成 IR→机器码链路。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 核心背景 | [01-background.md](./01-background.md) |
| §2 | 局部寄存器分配 | [02-local-regalloc.md](./02-local-regalloc.md) |
| §3 | 活性与存活范围 | [03-live-ranges.md](./03-live-ranges.md) |
| §4 | 图着色全局分配 | [04-graph-coloring.md](./04-graph-coloring.md) |
| §5 | 高级话题 | [05-advanced-topics.md](./05-advanced-topics.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch13 | 对照 |
|----------|------|
| ch1 后端三难点 | [ch1 §4.2](../chapter01_overview/04-translation-pipeline-example.md) |
| ch9 活跃变量 | [ch9 §1](../chapter09_dataflow/01-iterative-dataflow.md) — LIVE 是存活范围基础 |
| ch12 调度矛盾 | [ch12 §3](../chapter12_instr-sched/03-scheduling-vs-regalloc.md) |
| ch11 虚拟寄存器 | [ch11](../chapter11_instr-select/README.md) — 选择后仍用 vreg |
| RFR 栈/内存 | [03-2 OS 内存布局](../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md) |
| LLVM | `RegAllocGreedy` · 图着色族 |

---

## 逻辑脉络

背景与术语 → 块内启发式 → 跨块 LIVE → 图着色 + coalesce → 机器约束。

---

## 速记

## 本章速记

```text
§1  reg≪mem · spill · Allocation vs Assignment · 寄存器类
§2  块内 自顶向下/自底向上 · next-use
§3  LIVE · Live Range · 跨块冲突边
§4  冲突图 k-着色 · spill 代价 · Coalescing 删 copy
§5  预着色/机器约束 · splitting · Part IV 收官
```

---

## 三句背诵

1. **Regalloc = 虚拟寄存器映射到 k 个物理寄存器，不够就 spill。**
2. **冲突图：同时活跃不能同色；k-着色启发式 + spill 代价。**
3. **Coalesce 不冲突的 copy 对 — 同色消 mov。**

---

## 后端三关（全书）

| 章 | 任务 |
|:--:|------|
| 11 | 选指令 |
| 12 | 排顺序 |
| 13 | 分寄存器 |

---

## 自测

- [ ] Allocation 与 Assignment
- [ ] Live range 与冲突边
- [ ] 简化栈着色四步概念
- [ ] Coalescing 条件
- [ ] ch12 与 ch13 如何互相影响

---

## 阅读进度

- [x] ch13 寄存器分配 — **Part IV 完成 · 正文 13 章完结**
- [ ] 附录 A · B

