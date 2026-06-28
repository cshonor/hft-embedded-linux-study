## 1. 本章定位

> **ULK Ch 7 Process Scheduling** · 营造「多进程同时运行」的错觉

---

### 一、本章讲什么

- **何时**切换进程、**选谁**上 CPU
- 2.6 **O(1) 调度器**：`runqueue`、活动/过期数组
- 普通进程 vs **实时进程**（FIFO / RR）
- **`schedule()`** 核心路径
- SMP **负载均衡**、CPU 亲和性

Ch 3 讲进程与切换；Ch 6 讲 tick；本章讲 **tick 如何驱动调度决策**。

> **Modern 对照：** 5.x+ 默认 **CFS**（完全公平调度），O(1) 已移除；读 ULK 抓 **概念**（优先级、时间片、runqueue、need_resched），对照现网 `sched/` 源码。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-调度策略与抢占.md) | 可抢占、`TIF_NEED_RESCHED`、静/动态优先级 |
| [3](./section-3-调度器数据结构.md) | per-CPU runqueue、活动/过期数组 |
| [4](./section-4-调度算法与核心函数.md) | `scheduler_tick`、`schedule`、`try_to_wake_up` |
| [5](./section-5-SMP运行队列平衡.md) | 调度域、load_balance |
| [6](./section-6-调度相关系统调用.md) | nice、affinity、sched_setscheduler |

---

### 三、在 Linux 链上的位置

```
Ch 3  进程 / switch_to
Ch 5  内核抢占
Ch 6  tick / jiffies
Ch 7  调度（本章）
Ch 10 nice / sched_* syscall
Ch 15 HFT 绑核、SCHED_FIFO
```

---

← [Ch 7 导读](../README.md) · 下一节 [2. 调度策略与抢占](./section-2-调度策略与抢占.md)
