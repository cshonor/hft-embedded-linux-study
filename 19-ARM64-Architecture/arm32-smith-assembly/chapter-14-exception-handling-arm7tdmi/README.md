# Ch 14 · 异常处理：ARM7TDMI

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Exception Handling: ARM7TDMI

---

## 本章定位

| | |
|---|---|
| **角色** | **选读** — **ARM7 经典异常模型**（CPSR/SPSR/向量/FIQ/VIC） |
| **核心模式** | 硬件四步序列 · **向量存 `B`** · **`SUBS pc,lr,#n`** · **VIC 向量 IRQ** |
| **前置** | [Ch2 七种模式](../chapter-02-programmers-model/notes/section-2-3-arm7tdmi.md) · [Ch13 堆栈](../chapter-13-subroutines-stacks/) |
| **实操对照** | **Cortex-M** → [Ch15](../chapter-15-exception-handling-v7m/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§14.1** | 简介 | [notes/section-14-1-intro.md](./notes/section-14-1-intro.md) |
| **§14.2** | 中断 | [notes/section-14-2-interrupts.md](./notes/section-14-2-interrupts.md) |
| **§14.3** | 错误条件 | [notes/section-14-3-error-conditions.md](./notes/section-14-3-error-conditions.md) |
| **§14.4** | 异常序列 | [notes/section-14-4-exception-sequence.md](./notes/section-14-4-exception-sequence.md) |
| **§14.5** | 向量表 | [notes/section-14-5-vector-table.md](./notes/section-14-5-vector-table.md) |
| **§14.6** | 处理程序与优先级 | [notes/section-14-6-handlers-priority.md](./notes/section-14-6-handlers-priority.md) |
| **§14.7** | 基础机制小结 | [notes/section-14-7-mechanism.md](./notes/section-14-7-mechanism.md) |
| **§14.8** | 处理异常的程序 — 复位 · 未定义 · VIC · 中止 · SVC | [notes/section-14-8-handler-code.md](./notes/section-14-8-handler-code.md) |
| **§14.9** | 练习题 | [notes/section-14-9-exercises.md](./notes/section-14-9-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **中断 (IRQ/FIQ/SVC)** vs **错误 (Und/Prefetch/Data Abort)**
- [ ] 背 **硬件异常四步** 与 **优先级**（Reset 最高 …）
- [ ] 解释向量表为何放 **`B`/`LDR PC`**，FIQ **0x18 表末** 布局
- [ ] 写 **IRQ handler** 骨架：**压栈 → 服务 → `SUBS pc, lr, #4`**
- [ ] 说明 **VIC** 如何避免 IRQ 公共 handler **轮询**
- [ ] 对比 [Ch15 v7-M](../chapter-15-exception-handling-v7m/)：**NVIC** vs **VIC**

---

← [Ch 13](../chapter-13-subroutines-stacks/) · 下一章 [Ch 15](../chapter-15-exception-handling-v7m/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
