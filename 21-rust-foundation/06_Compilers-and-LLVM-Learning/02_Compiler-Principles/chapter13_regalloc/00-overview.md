# 第 13 章 · 寄存器分配 · 本章定位

← [本章目录](./README.md) · 上一章：[ch12 指令调度](../chapter12_instr-sched/README.md) · 下一节：[01-background.md](./01-background.md)

---

```text
ch11  选指令（虚拟寄存器）
  ↓
ch12  排顺序（可能抬高 LIVE）
  ↓
ch13  分物理寄存器 / spill  ← 本章 · Part IV 收官
  ↓
  可链接目标码（+ 调用约定 · prologue/epilogue）
```

| ch1 三阶段 | 本章 |
|------------|------|
| **Back end 第三关** | [§4.2 寄存器分配](../chapter01_overview/04-translation-pipeline-example.md) |

**核心矛盾**：IR/汇编层假设**无限虚拟寄存器**；CPU 只有 **k 个**物理寄存器 — 多出来的必须 **spill 到内存**。

**全书主线**：ch1 总览 → 前端 → IR → 优化 → **ch11～13 代码生成** — 至此 **13 章正文**闭环。

---

## 与 ch12 再遇

调度为填 stall 会**拉长**值的生命期 → regalloc 更难 — 见 [ch12 §3](../chapter12_instr-sched/03-scheduling-vs-regalloc.md)。

工业界 **Pass 顺序** 常多次调度 + 分配，非教科书线性一次。
