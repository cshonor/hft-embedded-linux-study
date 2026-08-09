## Ch8 完整概述 · 分支与循环

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Branches and Loops · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **控制流** | **B/BL/CBZ** + While/For/Do-While |
| **少分支** | **条件后缀 / IT** · **向下计数** · **展开** |
| **流水线意识** | 分支 = flush — 与性能优化同源 |

**前置：** [Ch7 标志与 CMP](../chapter-07-integer-logic-arithmetic/notes/section-0-本章完整概述.md) · [Ch3 阶乘 IT](../chapter-03-instruction-sets-v4t-v7m/notes/section-3-4-example-factorial.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **分支指令** | §8.2 | [section-8-2-branches.md](./section-8-2-branches.md) |
| **三种循环** | §8.3 | [section-8-3-loops.md](./section-8-3-loops.md) |
| **条件执行/IT** | §8.4 | [section-8-4-conditional.md](./section-8-4-conditional.md) |
| **循环展开** | §8.5 | [section-8-5-straight-line.md](./section-8-5-straight-line.md) |

---

### 三、知识流（口述版）

```
Ch7 标志 ← CMP/SUBS
        ↓
B{cond} / BL 子程序
        ↓
循环：SUBS 向下 + BNE；或 CBZ 短测
        ↓
短 if：ARM 后缀 或 IT 块（免分支）
        ↓
极热路径：循环展开（无 BNE）
        ↓
Ch13：BL 与栈 · Ch16：MMIO 轮询循环
```

---

### 四、ARM7 vs M4 速查

| | ARM7 ARM 模式 | Cortex-M Thumb-2 |
|---|---------------|------------------|
| 短 if | `SUBGT` 等 | `IT` + `SUBGT` |
| 小循环零测 | CMP+B | **CBZ/CBNZ** |
| 调用 | **BL** | **BL** |

---

### 五、下一章（按 OUTLINE）

浮点 **Ch9–11 跳过** → 精读 **Ch13 栈** / **Ch16 MMIO** 或继续顺序阅读
