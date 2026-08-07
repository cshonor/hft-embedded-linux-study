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
Ch 16 HFT 绑核、SCHED_FIFO
```

### 常见陷阱

1. 把 ULK 讲的 O(1) 调度器当现代版——2.6.23 起 CFS 取代 O(1)，6.6 起 EEVDF 取代 CFS
2. 以为 nice 值直接决定 CPU 时间比例——nice 通过 `static_prio` 查 `sched_prio_to_weight[]` 表得到权重，权重决定比例
3. 混淆 SCHED_OTHER 和 SCHED_FIFO/RR——OTHER 是普通分时（CFS/EEVDF），FIFO/RR 是实时策略（RT 调度器）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK 讲的 O(1) 调度器在现代内核中还存在吗？

<details><summary>答案</summary>

不存在。演进链：O(1) 调度器（2.6.0-2.6.22）→ CFS（2.6.23-6.5）→ EEVDF（6.6+）。O(1) 用优先级数组 + 时间片，CFS 用 vruntime + 红黑树，EEVDF 用虚拟截止时间 + eligibility。ULK 的 `recalc_task_prio()`、优先级数组等概念已完全过时。

</details>

**Q2.** CFS 和 EEVDF 的核心理念有什么区别？

<details><summary>答案</summary>

CFS（完全公平）：按权重瓜分 CPU，vruntime 记账，选 vruntime 最小的。目标：完美公平。EEVDF（最早合格虚拟截止时间）：每个任务有资格时间（eligibility）和截止时间（deadline），选最早截止且有资格的。目标：公平 + 延迟保证。EEVDF 在延迟敏感场景（如交互/媒体）表现更好，且解决了 CFS 的某些公平性 corner case。

</details>

**Q3.** HFT 应该用哪个调度策略？

<details><summary>答案</summary>

`SCHED_FIFO`（实时，优先级 1-99）。交易线程设 `SCHED_FIFO` + 绑核 + `mlockall`，确保不被普通进程抢占。注意：① `SCHED_FIFO` 线程不会自动让出 CPU（除非阻塞/主动 yield/更高优先级抢占）。② 需 `CAP_SYS_NICE` 或 root。③ RT 线程 bug 可能锁死 CPU，设 `rlimit_rttime` 限制。④ `isolcpus` 隔离防其他 RT 任务竞争。

</details>

</details>

---

← [Ch 7 导读](../README.md) · 下一节 [2. 调度策略与抢占](./section-2-调度策略与抢占.md)
> ↔ [LKD Ch04 §4.1 多任务与调度器演进](../../../07-linux-kernel/00_Book_3rd_Notes/chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md)
