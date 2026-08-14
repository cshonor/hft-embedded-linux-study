# 第 12 章 · 指令调度 · §3 调度与寄存器分配的矛盾

← [本章目录](./README.md) · 上一节：[02-list-scheduling.md](./02-list-scheduling.md) · 下一节：[04-advanced-topics.md](./04-advanced-topics.md)

---

全书核心理念之一：**Phase Ordering Problem** — 调度与 [ch13 寄存器分配](../chapter13_regalloc/) 相互**牵制**。

---

## 冲突机制

| 调度器倾向 | 后果 |
|------------|------|
| **交叉混合**无关指令 | 提高 ILP、隐藏 stall |
| 更多值**同时 live** | 需要**更多物理寄存器** |

```text
// 未调度：r1 用完即 dead
load r1 … ; use r1 ; load r2 …

// 调度后：r1 与 r2 生命周期重叠
load r1 … ; load r2 … ; use r1 ; use r2
              ↑ LIVE 集合变大
```

**溢出（Spill）**：寄存器不够 → **store 到栈** / **load 回来** — 内存延迟常**远大于**省下的 stall。

---

## 可能净亏

| 情况 | 结果 |
|------|------|
| 激进调度 + 少寄存器 ISA | spill 增加 → **整体变慢** |
| 保守调度 + 好 regalloc | 少 spill，但 stall 多 |

→ 不能「先极致调度再 regalloc」或反之**简单串联** — 需**权衡**或**迭代**。

---

## 工程策略（概念）

| 策略 | 说明 |
|------|------|
| **Pre-RA 调度** | 先用虚拟寄存器调度 — 不知真实压力 |
| **Post-RA 调度** | regalloc 后再调度 — 只能动无 spill 风险的序 |
| **二者结合** | LLVM 等多次调度 Pass |
| **Spill-aware 调度** | 优先级考虑寄存器压力 |
| **分配器反馈** | 若 spill 过多，回退或重调 |

**本书顺序** ch12 → ch13 是**教学顺序**；工业实现 **Pass 顺序因目标而异**。

---

## 与 ch9 活跃变量

[活跃变量分析](../chapter09_dataflow/01-iterative-dataflow.md) 度量 **LIVE 集合大小** — 调度直接**增大** LIVE → regalloc 输入变难。

---

## HFT 联想

x86-64 寄存器较多，调度/regalloc 冲突略缓；嵌入式 / 旧 ISA 更敏感。热点仍应看 **spill 是否出现在循环内**。

---

## 自测

- [ ] 为何调度会增加同时 live 的变量
- [ ] spill 如何抵消调度收益
- [ ] 举一种工业界处理 phase ordering 的做法
