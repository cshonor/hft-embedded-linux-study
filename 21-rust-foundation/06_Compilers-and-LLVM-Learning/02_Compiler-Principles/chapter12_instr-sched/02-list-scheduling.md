# 第 12 章 · 指令调度 · §2 列表调度

← [本章目录](./README.md) · 上一节：[01-motivation-and-problem.md](./01-motivation-and-problem.md) · 下一节：[03-scheduling-vs-regalloc.md](./03-scheduling-vs-regalloc.md)

---

编译器中最实用、最经典的调度技术 — **List Scheduling** 及其变体。

---

## 基本原理（贪心启发式）

```text
1. 建依赖 DAG
2. 维护「就绪」集合 Ready（所有前驱已调度）
3. 按优先级从 Ready 选一条 emit
4. 更新 Ready，直到空
```

| 要素 | 说明 |
|------|------|
| **Ready 列表** | 当前可合法发射的指令 |
| **优先级函数** | 打破平局、导向好序 |
| **资源模型** | 功能单元数、发射宽度（简化） |

**非最优** — 但 **O(n log n)** 量级、实现清晰，工业标配。

---

## 优先度方案（Priority Schemes）

常见启发式（可组合）：

| 优先级 | 直觉 |
|--------|------|
| **关键路径 / 最长路径优先** | 延迟链上的指令先排 — 缩短 makespan |
| **最长 latency 优先** | 慢指令（load、div）尽早启动 |
| **最多后继优先** | 解锁更多 Ready 节点 |
| **高度 / 层级** | 离出口近或依赖深者优先 |

```text
// 概念：load 延迟 3，应尽早 issue
Ready: { load A, add B, mul C }
 pick → load A   （关键路径 / 高 latency）
```

**调参**：不同 ISA / microarch 权重不同 — benchmark 驱动。

---

## 前向 vs 向后列表调度

| 方向 | 从哪出发 | 典型场景 |
|------|----------|----------|
| **前向（Forward）** | 块**入口** → 出口 | 最常见；按程序序填隙 |
| **向后（Backward）** | 块**出口** → 入口 | 某些延迟模型、资源约束 |

二者都是拓扑序上的贪心 — **方向**影响优先级定义与 Ready 初始化。

---

## 基本块内调度

默认 scope = **单个基本块**：

| 优点 | 局限 |
|------|------|
| DAG 无循环边，算法简单 | 不能跨分支移动 |
| 与 ch11 块内指令序列自然衔接 | 块边界处仍有 stall |

→ 更大 scope 见 [§4](./04-advanced-topics.md)。

---

## LLVM / 工程

- **SelectionDAG 调度** + **MachineScheduler**（pre-RA / post-RA）。
- **Itinerary / SchedModel**：TableGen 描述延迟与资源。
- `rustc` → LLVM 后端自动调度；读汇编时可对比 `-C opt-level` 下指令间距。

---

## 自测

- [ ] Ready 列表在算法中的作用
- [ ] 举一种优先级启发式及理由
- [ ] 前向与向后调度各适合什么直觉
