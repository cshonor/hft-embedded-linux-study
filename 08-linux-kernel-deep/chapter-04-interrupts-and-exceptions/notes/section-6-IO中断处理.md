## 6. I/O 中断处理

> 多设备可能 **共享 IRQ 线**（尤其 PCI）— 处理程序须足够灵活

---

### 一、ISR 四步（典型）

所有 I/O 中断处理程序基本流程：

| 步骤 | 动作 |
|------|------|
| 1 | 在内核栈保存 **IRQ 号** 和 **寄存器** |
| 2 | 向 **PIC/APIC** 发送 **ACK**（确认） |
| 3 | 执行该 IRQ 关联的所有设备的 **ISR** |
| 4 | 跳转退出 |

内核主要通过 **`do_IRQ()`** 调度各设备注册的 ISR。

---

### 二、PIC vs APIC

| 组件 | 时代/场景 |
|------|-----------|
| **PIC** | 传统可编程中断控制器 |
| **APIC / IOAPIC** | **多处理器** 系统中断分发 — SMP 标配 |

→ SMP 与同步：[Ch 5](../chapter-05-kernel-synchronization.md) · [Ch 1](../chapter-01-introduction/notes/section-2-Linux与Unix比较.md) SMP 介绍

---

### 三、上半部 vs 下半部

ISR 里应 **尽量少干活** — 耗时工作延后到 softirq / tasklet / workqueue（[section-7](./section-7-可延迟函数与工作队列.md)）。

→ 驱动模型：[Ch 13 I/O 架构](../chapter-13-io-architecture.md)

---

← [5. 异常处理](./section-5-异常处理.md) · 下一节 [7. 可延迟函数与工作队列](./section-7-可延迟函数与工作队列.md)
