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

交叉：[05 LKD Ch 7–8](../../../05-Linux-Kernel-Development/) · [04 BPF 内核路径](../../../04-BPF-Performance-Tools/) · [14 DPDK 绕过内核](../../../14-DPDK-Low-Latency-Network/)

---

← [Ch 4 导读](../README.md) · 下一节 [2. 中断与异常分类](./section-2-中断与异常分类.md)
