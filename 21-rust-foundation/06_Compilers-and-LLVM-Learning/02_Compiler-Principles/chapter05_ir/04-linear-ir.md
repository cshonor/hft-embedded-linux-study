# 第 5 章 · 中间表示 · §4 线性 IR

← [本章目录](./README.md) · 上一节：[03-graphical-ir.md](./03-graphical-ir.md) · 下一节：[05-ssa-form.md](./05-ssa-form.md)

---

## 栈机器代码（Stack-machine code）

| 假设 | 指令隐式操作**操作数栈** |
|------|---------------------------|
| 例 | `push` · `add` — 弹栈运算、压结果 |

**特点**：IR **极其紧凑**；VM 实现简单。

| 实例 | 说明 |
|------|------|
| **Java 字节码** | JVM 栈机 |
| **PostScript** | 栈式语言 |
| **clox** | 值栈 + 字节码 → [CI ch14 Chunk](../../../01_Crafting-Interpreters/part03_clox/chapter14_chunks-of-bytecode/README.md) |

---

## 三地址代码（Three-address code）

每条指令大致：**一个操作符 + 两个源 + 一个目标**。

```text
x ← y op z
x ← y              （一元 / 拷贝）
x ← op y           （一元运算）
```

| 优点 | 说明 |
|------|------|
| **便于分析** | 显式 def-use，利于数据流 |
| **便于寄存器分配** | 虚拟寄存器 ↔ 三地址临时变量 |

---

## ILOC（本书教学汇编）

- 橡书使用的典型**三地址**线性 IR。
- 与 [ch1 示例 `w ← w * 2 * x * y * z`](../chapter01_overview/04-translation-pipeline-example.md) 后端代码生成衔接。
- ch11～13 **指令筛选 / 调度 / 寄存器分配** 常以 ILOC 为输入。

---

## 栈机 vs 三地址

| | **栈机** | **三地址** |
|---|----------|------------|
| 密度 | 高 | 较低 |
| 分析/优化 | 较难（隐式栈） | **易**（显式名字） |
| 典型用途 | VM、解释器 | 优化编译器 IR 层 |

**clox**：栈机字节码（快写 VM）；**LLVM**：三地址式 SSA（深优化）。
