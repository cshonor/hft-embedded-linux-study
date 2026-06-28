## 1. 本章定位

> **ULK Ch 5 Kernel Synchronization** · 保护共享数据，避免竞态

---

### 一、本章讲什么

Ch 4 讲了中断/异常导致 **内核控制路径交错**。本章回答：

- 交错访问共享数据时如何 **同步**？
- **内核抢占** 对同步有什么影响？
- **自旋锁 vs 信号量 vs RCU** 何时用哪个？

单处理器和多处理器 **都需要** 同步 — SMP 下竞态更常见，但中断嵌套在单核上同样危险。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-内核抢占.md) | 2.6 内核抢占、不可抢占场景 |
| [3](./section-3-基础同步原语.md) | per-CPU、原子、屏障、关中断 |
| [4](./section-4-自旋锁.md) | spinlock、读写自旋锁 |
| [5](./section-5-顺序锁与RCU.md) | seqlock、RCU |
| [6](./section-6-信号量与完成变量.md) | semaphore、completion |
| [7](./section-7-选型与实例.md) | 按访问者选型、BKL/refcount 等实例 |

---

### 三、在 Linux 链上的位置

```
Ch 4  中断与异常  — 控制路径为何交错
Ch 5  内核同步    — 交错时如何保护数据（本章）
Ch 7  进程调度    — 抢占与 schedule() 触发点
Ch 3  等待队列    — 信号量睡眠/唤醒
```

交叉：[05 LKD Ch 9–10](../../../05-Linux-Kernel-Development/) · HFT 热路径：**spinlock 持锁时间** 直接影响延迟

---

← [Ch 5 导读](../README.md) · 下一节 [2. 内核抢占](./section-2-内核抢占.md)
