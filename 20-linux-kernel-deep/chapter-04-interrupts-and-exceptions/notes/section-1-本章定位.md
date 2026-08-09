## 1. 本章定位

> **ULK Ch 4 Interrupts and Exceptions** · 底层硬件事件如何驱动内核

---

### 一、本章讲什么

中断和异常是 **改变处理器正常指令执行顺序** 的特殊信号。本章解析：

- 80x86 硬件机制（IDT、IRQ、异常向量）
- Linux 如何 **高效处理** 中断/异常
- **上半部 / 下半部** 分工（ISR vs softirq/workqueue）
- 返回用户态前的 **调度与信号** 检查

Ch 3 讲进程切换；本章讲 **什么事件会打断进程** 并进入内核态。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-中断与异常分类.md) | 中断 vs 异常；Fault/Trap/Abort |
| [3](./section-3-IDT与门描述符.md) | IDT、中断门/陷阱门/系统门 |
| [4](./section-4-控制路径嵌套.md) | 内核控制路径、嵌套规则 |
| [5](./section-5-异常处理.md) | 信号、Kernel oops |
| [6](./section-6-IO中断处理.md) | `do_IRQ`、ISR 四步 |
| [7](./section-7-可延迟函数与工作队列.md) | softirq、tasklet、workqueue |
| [8](./section-8-中断返回.md) | `ret_from_intr`、调度、信号 |

---

### 三、在 Linux 链上的位置

```
Ch 3  进程        — 被中断/异常打断的对象
Ch 4  中断与异常  — 进内核的硬件/软件入口（本章）
Ch 5  内核同步    — 控制路径交错时的锁
Ch 7  调度        — 中断返回时可能触发 reschedule
Ch 10 系统调用    — 编程异常 / int 0x80 路径
Ch 13 I/O 架构    — 设备驱动与 IRQ
```

交叉：[05 LKD Ch 7–8](../../../05-linux-kernel/) · [04 BPF 内核路径](../../../17-bpf-observability/) · [14 DPDK 绕过内核](../../../15-dpdk/)

### 常见陷阱

1. 把 ULK 的 IDT 结构直接用于 64 位分析——x86-64 IDT 条目格式不同（16 字节，含 IST 字段），且 `int $0x80` 不再是 syscall 入口
2. 混淆「中断」和「异常」——中断是异步的（硬件触发），异常是同步的（指令执行触发）
3. 以为中断处理不能睡眠——传统中断（hard IRQ）不能睡眠，但 threaded IRQ 和 workqueue 可以

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK Ch4 讲的中断处理框架在现代内核中最大的变化是什么？

<details><summary>答案</summary>

① x86-64 用 `IDTENTRY` 宏统一管理 IDT 条目，取代手写汇编 stub。② `int $0x80` 被 `syscall` 指令取代作为系统调用入口。③ threaded IRQ（`request_threaded_irq()`）允许中断处理函数在内核线程中运行，可以睡眠。④ `irq_desc` 层级从全局数组改为 per-domain 的 IRQ domain 树。

</details>

**Q2.** hard IRQ 为什么不能睡眠？threaded IRQ 怎么解决这个限制？

<details><summary>答案</summary>

hard IRQ 运行在中断上下文（无 `task_struct`、无可调度实体），调度器无法切换。睡眠需要 `schedule()`，会 panic。threaded IRQ 把中断处理拆成两半：hard IRQ 只确认硬件 + 唤醒内核线程，实际处理在线程中运行（有 `task_struct`，可调度可睡眠）。用 `request_threaded_irq(dev, hard_fn, thread_fn, flags, ...)` 注册。

</details>

**Q3.** HFT 中如何减少中断对热路径的干扰？

<details><summary>答案</summary>

① `irqbalance` off + 手动绑中断到非交易核（`/proc/irq/[n]/smp_affinity`）。② `napi` 轮询模式代替中断驱动收包。③ DPDK 完全绕过内核中断，用用户态轮询。④ `isolcpus` + `nohz_full` 减少定时器中断。

</details>

</details>

---

← [Ch 4 导读](../README.md) · 下一节 [2. 中断与异常分类](./section-2-中断与异常分类.md)
