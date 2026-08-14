# 第 13 章 · 寄存器分配 · §3 活性与存活范围

← [本章目录](./README.md) · 上一节：[02-local-regalloc.md](./02-local-regalloc.md) · 下一节：[04-graph-coloring.md](./04-graph-coloring.md)

---

跨越基本块边界 — 全局分配的数据流基础（与 [ch9 活跃变量](../chapter09_dataflow/01-iterative-dataflow.md) 直接衔接）。

---

## 活性（Liveness）

| 定义 | 变量 **v** 在程序点 **p** **活跃** ⇔ 从 **p** 到出口存在路径，**p** 之后 **v 被读** 且中间未被重新定义 |
|------|----------------------------------------------------------------------------------------------------------|

**反向数据流**：

```text
LIVEIN(B)  = use(B) ∪ (LIVEOUT(B) − def(B))
LIVEOUT(B) = ⋃ LIVEIN(S)   （S 为后继）
```

→ ch9 已推导方程；regalloc **整函数**跑一遍得每点 LIVE 集。

---

## 存活范围（Live Range）

从 **v 某次 def** 到 **该 def 产生的值最后一次 use** 之间的程序片段 — 常跨**多个基本块**。

```text
BB1:  v = …        ← range 起点
BB2:  … use v …
BB3:  use v        ← range 终点（此 def 的值）
```

| 概念 | 说明 |
|------|------|
| **SSA 名** | 每个 def 一个名 → 范围清晰（ch9 重命名） |
| **非 SSA** | 范围可能合并/分裂 — 构造更烦 |

**冲突**：两范围 **在同一程序点同时活跃** → 不能同色（同 preg）。

---

## 从 LIVE 到冲突边

```text
对每个程序点 P：
  LIVE(P) 中任意两范围 {a,b}  →  冲突图加边 (a,b)
```

**全局视角**：块内局部启发式看不到的跨块重叠，此处一次性捕获。

---

## 与 ch12

[调度](../chapter12_instr-sched/03-scheduling-vs-regalloc.md) 拉长范围 → LIVE 集变大 → 冲突图更密 → spill ↑。

---

## 自测

- [ ] Live range 与 ch9 LIVEOUT 的关系
- [ ] 为何 SSA 简化存活范围构造
- [ ] 什么情况下两变量冲突
