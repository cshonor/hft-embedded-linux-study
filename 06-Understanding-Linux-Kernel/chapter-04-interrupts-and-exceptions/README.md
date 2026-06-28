# Ch 4 中断与异常 · Interrupts and Exceptions

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 硬件与内核交互的核心机制 — 改变 CPU 正常指令流

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-chapter-scope.md](./notes/section-1-chapter-scope.md) |
| 2. 中断与异常的区别 | [notes/section-2-interrupts-vs-exceptions.md](./notes/section-2-interrupts-vs-exceptions.md) |
| 3. IDT 与三种门 | [notes/section-3-idt-and-gates.md](./notes/section-3-idt-and-gates.md) |
| 4. 内核控制路径嵌套 | [notes/section-4-nested-kernel-paths.md](./notes/section-4-nested-kernel-paths.md) |
| 5. 异常处理 | [notes/section-5-exception-handling.md](./notes/section-5-exception-handling.md) |
| 6. I/O 中断处理 | [notes/section-6-io-interrupt-handling.md](./notes/section-6-io-interrupt-handling.md) |
| 7. Softirq · Tasklet · 工作队列 | [notes/section-7-softirqs-and-workqueues.md](./notes/section-7-softirqs-and-workqueues.md) |
| 8. 从中断返回 | [notes/section-8-return-from-interrupt.md](./notes/section-8-return-from-interrupt.md) |

---

## 相关

- 上一章：[chapter-03-processes/](../chapter-03-processes/)
- 下一章：[chapter-05-kernel-synchronization/](../chapter-05-kernel-synchronization/)
- 衔接：[chapter-07-process-scheduling.md](../chapter-07-process-scheduling.md) · [chapter-10-system-calls.md](../chapter-10-system-calls.md) · [chapter-13-io-architecture.md](../chapter-13-io-architecture.md)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
