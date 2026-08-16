# 第 13 章 · 寄存器分配 · §4 图着色全局分配

← [本章目录](./README.md) · 上一节：[03-live-ranges.md](./03-live-ranges.md) · 下一节：[05-advanced-topics.md](./05-advanced-topics.md)

---

现代优化编译器的**主流**全局方案 — **Graph Coloring Register Allocation**。

---

## 冲突图（Interference Graph）

| 元素 | 含义 |
|------|------|
| **节点** | 存活范围（或 SSA 名） |
| **边** | 两范围**同时活跃** → 不能同寄存器 |

**目标**：用 **k 色**（k = 可用物理寄存器数）着色，**相邻节点不同色**。

```text
     [v1]────[v2]
       |   X   |
     [v3]────[v4]

k=3 色可行 → 4 个值用 3 个 preg + 1 spill
```

---

## 着色算法（启发式）

**k-着色是 NP-complete** — 用 **Chaitin 式** 自顶向下 / 自底向上启发式：

| 阶段 | 做法 |
|------|------|
| **简化** | 删度数 < k 的节点（压栈） |
| **溢出候选** | 度 ≥ k 时选 ** spill 代价最小** 节点标记 |
| **着色** | 出栈时赋色；无可用色 → **spill** |

**自顶向下着色** vs **自底向上着色**：栈构建与 spill 决策顺序不同 — 影响 spill 质量。

---

## 评估全局 Spill 代价

当 k 色不够，必须 spill — 选 **损失最小** 的节点：

| 因素 | 权重直觉 |
|------|----------|
| **使用频率** | 少 use → 优先 spill |
| **循环深度** | 循环内 use → 代价高，尽量保留 |
| **def-use 距离** | 跨度大且少 use → 可 spill |
| **是否已 memory 值** | 已在栈上则代价低 |

```text
spill_cost(v) = Σ uses × loop_depth_weight …
```

**全局**：一次 spill 决策影响整图 — 可迭代「spill → 重建图 → 再着色」。

---

## 接合存活范围（Coalescing）

**极强**的优化 — 消除多余 **copy**：

| 条件 | 操作 |
|------|------|
| `x = y`（copy）且 **x 与 y 范围不冲突** | 合并为一个节点 |
| 着色后 | 同色 → **删掉 copy 指令** |

```text
// 前
mov r1, r2    // y→x

// coalesce 后（同色）
// （指令消失）
```

**保守接合**：仅当合并后 **不会** 人为抬高度数导致无谓 spill（Briggs / George 等准则）。

与 ch10 **拷贝传播** 协同 — 更多 copy 变成可 coalesce 候选。

---

## 算法总览

```text
建 LIVE → 建冲突图 → （copy coalesce）→ 简化/着色
    ↓ spill
插入 load/store → 可能再调度（ch12）→ 最终汇编
```

**LLVM**：`RegAllocGreedy` — 图着色族 + live range 分裂 + coalesce。

---

## 自测

- [ ] 冲突图节点/边各代表什么
- [ ] k 色与 k 个物理寄存器的对应
- [ ] Coalescing 如何删 mov
- [ ] 为何图着色是 NP 完全仍够用
