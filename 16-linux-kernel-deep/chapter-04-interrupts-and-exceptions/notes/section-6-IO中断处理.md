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

→ SMP 与同步：[Ch 5](../../chapter-05-kernel-synchronization.md) · [Ch 1](../../chapter-01-introduction/notes/section-2-Linux与Unix比较.md) SMP 介绍

---

### 三、上半部 vs 下半部

ISR 里应 **尽量少干活** — 耗时工作延后到 softirq / tasklet / workqueue（[section-7](./section-7-可延迟函数与工作队列.md)）。

→ 驱动模型：[Ch 13 I/O 架构](../../chapter-13-io-architecture.md)

### 常见陷阱

1. 以为 IRQ 号就是硬件中断号——现代用 IRQ domain + 虚拟 IRQ 号（virq），硬件号和 Linux IRQ 号不同
2. 混淆 `request_irq()` 和 `request_threaded_irq()`——前者不能睡眠，后者可在内核线程中处理
3. 以为中断处理函数返回 IRQ_HANDLED 就完了——还要操作硬件 ACK/EOI，否则同一中断不会再触发

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 现代内核的 IRQ domain 机制解决了什么问题？

<details><summary>答案</summary>

ULK 时代 IRQ 号 = 硬件中断号，全局数组 `irq_desc[]` 直接索引。现代内核支持中断控制器级联（GIC → GPIO → MSI），硬件号可能冲突。IRQ domain 为每级中断控制器建立独立的号码空间，通过 `irq_domain_translate()` 将硬件号映射为唯一的 Linux virtual IRQ（virq）。`/proc/interrupts` 显示的是 virq。

</details>

**Q2.** `request_threaded_irq()` 相比 `request_irq()` 有什么优势？

<details><summary>答案</summary>

允许把中断处理拆成 hard IRQ（确认硬件 + 唤醒）和 thread_fn（实际处理，可睡眠）。优势：① thread_fn 可以做 I/O、分配内存、持 mutex。② 减少 hard IRQ 时间，降低中断延迟。③ RT 内核（PREEMPT_RT）强制所有中断线程化。劣势：增加一次唤醒 + 调度延迟。

</details>

**Q3.** HFT 中如何确保 NIC 中断不干扰交易核？

<details><summary>答案</summary>

① `ethtool -L eth0 combined 1` 减少中断队列数。② `cat /proc/irq/[n]/smp_affinity_list` 设为非交易核。③ `service irqbalance stop` 禁止自动迁移。④ 如果用内核网络栈，配置 RPS/RFS 把 softirq 也路由到非交易核。⑤ 最佳方案：DPDK 绕过内核，用户态轮询收包。

</details>

</details>

---

← [5. 异常处理](./section-5-异常处理.md) · 下一节 [7. 可延迟函数与工作队列](./section-7-可延迟函数与工作队列.md)
> ↔ [LKD Ch07 §7.2 中断处理程序](../../../05-linux-kernel/chapter-07-interrupts/notes/section-7.2-中断处理程序.md)
