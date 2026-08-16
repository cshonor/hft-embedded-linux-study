# Ch 4 中断与异常 · Interrupts and Exceptions

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 硬件与内核交互的核心机制 — 改变 CPU 正常指令流

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| IDT 门描述符 (x86-32) | x86-64 IDT 结构不同，中断入口路径重写 | [x86 interrupt handling](https://lwn.net/Articles/107554/) |
| `do_IRQ()` 路径 | 仍存在但路径简化，IRQ 堆栈处理变化 | [Interrupt handling in Linux](https://lwn.net/Articles/302043/) |
| IPI 机制 | 改用 `smp_call_function()` 系列 | [Kernel doc: IPI](https://docs.kernel.org/core-api/smp.html) |
| 中断线程化 | ULK3 时代无，现代内核支持 threaded IRQ | [Threaded interrupt handlers](https://lwn.net/Articles/302043/) |

> **原则**：ULK3 用来理解中断概念框架（IDT/门/上半部/下半部），现代实现查 bootlin 中断训练材料 + 源码 `kernel/irq/`。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 中断与异常的区别 | [notes/section-2-中断与异常分类.md](./notes/section-2-中断与异常分类.md) |
| 3. IDT 与三种门 | [notes/section-3-IDT与门描述符.md](./notes/section-3-IDT与门描述符.md) |
| 4. 内核控制路径嵌套 | [notes/section-4-控制路径嵌套.md](./notes/section-4-控制路径嵌套.md) |
| 5. 异常处理 | [notes/section-5-异常处理.md](./notes/section-5-异常处理.md) |
| 6. I/O 中断处理 | [notes/section-6-IO中断处理.md](./notes/section-6-IO中断处理.md) |
| 7. Softirq · Tasklet · 工作队列 | [notes/section-7-可延迟函数与工作队列.md](./notes/section-7-可延迟函数与工作队列.md) |
| 8. 从中断返回 | [notes/section-8-中断返回.md](./notes/section-8-中断返回.md) |

---

## 相关

- 上一章：[chapter-03-processes/](../chapter-03-processes/)
- 下一章：[chapter-05-kernel-synchronization/](../chapter-05-kernel-synchronization/)
- 衔接：[chapter-07-process-scheduling.md](../chapter-07-process-scheduling.md) · [chapter-10-system-calls.md](../chapter-10-system-calls.md) · [chapter-13-io-architecture.md](../chapter-13-io-architecture.md)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
