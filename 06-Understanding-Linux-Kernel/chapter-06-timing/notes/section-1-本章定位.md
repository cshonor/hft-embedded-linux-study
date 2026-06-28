## 1. 本章定位

> **ULK Ch 6 Timing Measurements** · 内核如何感知时间、调度未来任务

---

### 一、本章讲什么

- 内核与 **硬件时钟**（RTC、TSC、PIT、HPET…）如何交互
- **`jiffies`**、tick、计时架构
- **定时器中断**里更新系统时间、进程 CPU 统计
- **动态定时器**、微秒级 **udelay/ndelay**
- 用户态 **gettimeofday / setitimer / POSIX timers**

Ch 4 讲中断从哪来；本章讲 **定时器中断驱动的时间流** — 是 Ch 7 调度的时钟基础。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-硬件时钟与定时器.md) | RTC、TSC、PIT、HPET、ACPI PM timer |
| [3](./section-3-Linux计时架构.md) | timer_opts、`jiffies`、SMP 全局/局部 tick |
| [4](./section-4-更新时间与统计.md) | `update_times`、CPU 时间、NMI watchdog |
| [5](./section-5-软件定时器与延迟函数.md) | 动态定时器 tv1–tv5、`udelay`/`ndelay` |
| [6](./section-6-定时相关系统调用.md) | gettimeofday、setitimer、POSIX timers |

---

### 三、在 Linux 链上的位置

```
Ch 4  中断与异常  — 定时器 IRQ 是中断源之一
Ch 6  定时测量    — tick、jiffies、动态定时器（本章）
Ch 7  进程调度    — 时间片、need_resched 依赖 tick
Ch 10 系统调用    — gettimeofday 等入口
```

交叉：[08 TLPI](../../../08-The-Linux-Programming-Interface/) · HFT：**TSC/HPET**、**HZ**、busy-wait 延迟

---

← [Ch 6 导读](../README.md) · 下一节 [2. 硬件时钟](./section-2-硬件时钟与定时器.md)
