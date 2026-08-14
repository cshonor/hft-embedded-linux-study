# 第 11 章 · 指令筛选 · §4 高级话题

← [本章目录](./README.md) · 上一节：[03-peephole-optimization.md](./03-peephole-optimization.md) · 下一节：---

## 学习窥孔模式（Learning Peephole Patterns）

手写窥孔规则**费时且易漏** — 研究如何用**自动化**推导或学习模式：

| 思路 | 说明 |
|------|------|
| **从样例合成** | 给定 (低效, 高效) 指令对，归纳通用模式 |
| **搜索 / 枚举** | 在小窗口内枚举变换，benchmark 验证 |
| **机器学习** | 用训练集学替换规则（研究向；工业仍多 TableGen + 手工） |

**动机**：ISA 扩展频繁（SIMD、新寻址）— 降低规则维护成本。

---

## 生成指令序列（Instruction Sequence Generation）

针对**复杂结构**（调用约定、prologue/epilogue、除法、128 位乘、原子操作）：

| 问题 | 策略 |
|------|------|
| **单 IR 操作 → 多指令** | 宏展开 / 内置 **libcall** 序列 |
| **Calling convention** | 固定模板：保存 callee-saved、对齐栈、传参 |
| **非法类型宽度** |  legalize 为多条窄指令 |
| **条件码 vs 分支** | 选 `setcc`+`cmov` 或 `branch` 序列 — 与 ch12 调度交互 |

```text
// 概念：IR 一条 div 在目标上无单指令
sdiv %a, %b  →  expand to:  compare · branch · idiv · fixup
```

**与 tiling 关系**：大树模式可拆成**宏规则**（匹配根，展开为子序列）。

---

## 本章收束 → ch12 / ch13

| 阶段 | 本章产出 | 遗留问题 |
|------|----------|----------|
| **指令筛选** | 合法、较优的**指令序列** | 虚拟寄存器仍无限 |
| **ch12 调度** | 重排顺序减 stall | 仍假设寄存器已命名 |
| **ch13 寄存器分配** | 虚拟 reg → 物理 reg / spill | 可能插入 load/store |

```text
ch11 选什么指令  →  ch12 什么顺序  →  ch13 进哪个寄存器
```

**HFT**：热点函数最终汇编质量 = 三关叠加；`-C target-cpu=native` 影响**模式库与窥孔**选型。

---

## 自测

- [ ] 为何复杂除法不能单靠一条树模式
- [ ] 学习窥孔 vs 手写规则的权衡
- [ ] ch11 输出还缺什么才能直接跑在 CPU 上
