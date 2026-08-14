# 第 12 章 · 指令调度 · §4 高级话题

← [本章目录](./README.md) · 上一节：[03-scheduling-vs-regalloc.md](./03-scheduling-vs-regalloc.md) · 下一节：---

## 区域调度（Regional Scheduling）

超越**单一基本块**，在更大区域上调度：

| Scope | 说明 |
|-------|------|
| **超块 / Trace** | 热路径上连续块（常含分支预测方向） |
| **循环体** | 软件流水线（software pipelining）基础 |
| **EBB / Region** | 单入口多块 DAG |

**收益**：块边界处的 stall 也可部分隐藏。

**代价**：DAG 更大、控制流复杂、实现与 regalloc 交互更难。

```text
  BB1 ──→ BB2 ──→ BB3   （热 trace）
   └─ 区域调度：跨 BB1→BB2 填隙
```

---

## 上下文的复制（Context Duplication）

为**打破控制流限制**、暴露更多调度机会，编译器**复制**部分代码：

| 手段 | 目的 |
|------|------|
| **尾复制 / 块复制** | 汇合点前复制，使各前驱有独立调度窗口 |
| **循环 peeling / unrolling** | 与 ch10 [循环展开](../chapter10_scalar-opt/02-core-transformations.md) 协同 |
| **if-conversion 预备** | 减少分支，扩大调度区域 |

**权衡**：代码体积 ↑（I-cache 压力） vs 调度机会 ↑ — 与 ch8 [内联](../chapter08_intro-opt/05-interprocedural-opt.md) 类似的体积/速度 trade-off。

---

## 软件流水线（概念）

对**循环**重叠不同迭代的阶段 — 把循环体拆成 prologue / kernel / epilogue：

```text
iter i:   load │ compute │ store
iter i+1:       load    │ compute │ store
```

属区域调度深化 — 本书概述，细节可结合专用文献 / LLVM `ModuloSchedule`。

---

## 本章收束 → ch13

| ch12 交付 | ch13 接续 |
|-----------|-----------|
| 较优**指令顺序** | 虚拟 reg → 物理 reg |
| 可能已抬高寄存器压力 | **图着色 / 线性扫描** 处理 spill |

```text
ch11 选指令 → ch12 排顺序 → ch13 分寄存器 → 可执行机器码
```

---

## 自测

- [ ] 区域调度相对块内调度的利弊
- [ ] 代码复制为何能暴露调度机会
- [ ] 软件流水线与列表调度的关系（一句话）
