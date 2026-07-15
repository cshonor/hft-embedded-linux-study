# Ch 15 · 异常处理：v7-M

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Exception Handling: v7-M

---

## 本章定位

| | |
|---|---|
| **角色** | **选读**（**M4 裸机/RTOS 实操权重高**）— **Cortex-M 标准异常模型** |
| **核心模式** | **0x0=MSP · 向量=地址** · **硬件 8-word 栈帧** · **EXC_RETURN** · **NVIC** |
| **前置** | [Ch2 Cortex-M4](../chapter-02-programmers-model/notes/section-2-4-cortex-m4.md) · [Ch14 ARM7 对照](../chapter-14-exception-handling-arm7tdmi/) |
| **后续** | [Ch16 MMIO](../chapter-16-memory-mapped-peripherals/) — 外设 + 中断联调 |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§15.1** | 简介 | [notes/section-15-1-intro.md](./notes/section-15-1-intro.md) |
| **§15.2** | 操作模式与特权级别 | [notes/section-15-2-modes-privilege.md](./notes/section-15-2-modes-privilege.md) |
| **§15.3** | 向量表 | [notes/section-15-3-vector-table.md](./notes/section-15-3-vector-table.md) |
| **§15.4** | 堆栈指针 — MSP/PSP | [notes/section-15-4-stack-pointers.md](./notes/section-15-4-stack-pointers.md) |
| **§15.5** | 处理器出入栈序列 | [notes/section-15-5-stack-frames.md](./notes/section-15-5-stack-frames.md) |
| **§15.6** | 异常类型 — 硬故障 · 内存管理故障等 | [notes/section-15-6-fault-types.md](./notes/section-15-6-fault-types.md) |
| **§15.7** | 中断 — 基于 NVIC 的外部中断 | [notes/section-15-7-nvic.md](./notes/section-15-7-nvic.md) |
| **§15.8** | 练习题 | [notes/section-15-8-exercises.md](./notes/section-15-8-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **Thread/Handler** 与 **Privileged/Unprivileged（CONTROL）**
- [ ] 解释 **0x0=初始 MSP**、**0x4=Reset**；向量存 **地址** 非 `B`
- [ ] 列出 **硬件压栈 8 寄存器** 与 **EXC_RETURN + BX lr** 返回
- [ ] 区分 **MSP/PSP** 及 RTOS 中 **PendSV 换 PSP** 概念
- [ ] 背 **Reset/NMI/HardFault** 优先级；知 **UsageFault/MemManage/BusFault**
- [ ] 走通 **NVIC 配 Timer 中断** 流程（时钟 → 外设 → NVIC → ISR 清标志）
- [ ] 对照 [Ch14](../chapter-14-exception-handling-arm7tdmi/)：**VIC vs NVIC**

---

← [Ch 14](../chapter-14-exception-handling-arm7tdmi/) · 下一章 [Ch 16](../chapter-16-memory-mapped-peripherals/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
