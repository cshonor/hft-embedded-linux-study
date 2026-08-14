# 第 12 章 · 指令调度 · 本章定位

← [本章目录](./README.md) · 上一章：[ch11 指令筛选](../chapter11_instr-select/README.md) · 下一节：[01-motivation-and-problem.md](./01-motivation-and-problem.md)

---

```text
ch11  选「用什么指令」
  ↓
ch12  排「什么顺序执行」  ← 本章
  ↓
ch13  分「值进哪个寄存器」
```

| ch1 三阶段 | 本章 |
|------------|------|
| **Back end 第二关** | [§4.3 指令调度](../chapter01_overview/04-translation-pipeline-example.md) |

**输入**：ch11 产出的**机器指令序列**（常仍用虚拟寄存器）。

**输出**：**语义等价**、**顺序更优**的指令序列 — 减少 CPU **等待前序结果**的空转。

**约束**：数据依赖、控制依赖、内存依赖 — 重排不得违反可见语义。

---

## 与超标量 CPU

现代 CPU 可**并行发射**多条独立指令，但**真依赖**（RAW）仍须等待 — 调度要在依赖图与延迟模型间找近似最优序。
