## ① 中断的概念 · Interrupts

硬件用 **电子信号** **异步** 打断 CPU，引起内核注意。

| 概念 | 说明 |
|------|------|
| **IRQ（中断请求线）** | 每个中断对应 **唯一数字** — 内核区分设备 |
| **异步中断** | 外设随时到来 — 网卡、磁盘、键盘… |

#### 中断 vs 异常（同步陷阱）

| 类型 | 来源 | 时机 |
|------|------|------|
| **硬件中断（IRQ）** | 外设 | **异步** — 与当前指令无关 |
| **异常（Exceptions）** | CPU 执行指令 | **同步** — 缺页、非法指令、**syscall 陷入** |

```
异步：  指令流 ──► 指令 ──► [IRQ 插入] ──► ISR ──► 继续
同步：  指令 ──► 触发异常（如 page fault）──► 异常处理
```

→ [Ch 1](../../chapter-01-intro/) syscall vs 中断 · [Ch 5](../../chapter-05-system-calls/) 进程上下文

→ 教学对照：[01 Day 5 GDT/IDT](../../../../07-system-low-level-hands-on/02-30days-os/day-05-gdt-idt/) · [Day 7 PIC](../../../../07-system-low-level-hands-on/02-30days-os/day-07-fifo-mouse/)

---
