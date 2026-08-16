## ⑥ 动态定时器 · Dynamic Timers

**动态定时器** = 「**在 future_jiffies 时异步叫我**」— **内核版 one-shot alarm**，基于 **`struct timer_list`**（2.6 经典模型；现代并行 **hrtimer**）。

#### `struct timer_list` 要点

| 字段 / API | 说明 |
|------------|------|
| **`expires`** | 到期时刻（**jiffies** — 用 `time_after` 比较） |
| **`function`** | 回调 `void (*fn)(unsigned long)` |
| **`data`** | 传给回调的 **unsigned long**（常强转指针） |
| **`init_timer()` / `setup_timer()`** | 初始化（书中 API；新内核多用 **`timer_setup()`**） |
| **`add_timer()`** | 挂入 **全局/per-CPU 链表** |
| **`mod_timer()`** | **改期** — 未激活则等同 add；已激活则 **迁移** 到期点 |
| **`del_timer()`** | 删除 — **不保证** 回调未开始 |
| **`del_timer_sync()`** | **同步删除** — **等回调跑完**（可能睡眠） |

```c
static struct timer_list stats_timer;

void stats_fn(unsigned long unused)
{
    /* 快速统计；不可 sleep */
    mod_timer(&stats_timer, jiffies + msecs_to_jiffies(1000));
}

void init_stats_timer(void)
{
    setup_timer(&stats_timer, stats_fn, 0);
    mod_timer(&stats_timer, jiffies + HZ);  /* ~1s 后首次触发 */
}
```

#### 执行上下文 · `TIMER_SOFTIRQ`

| 事实 | 约束 |
|------|------|
| 在 **softirq**（`TIMER_SOFTIRQ`）里跑回调 | **原子上下文** — **禁止 sleep** |
| 与 hardirq | timer IRQ **只标记**；实际回调 **稍后** softirq |
| 回调耗时 | 阻塞其他 timer → **系统级延迟** |

```
timer IRQ: 标记到期 timer
    ▼
raise TIMER_SOFTIRQ
    ▼
run_timer_softirq(): 逐个 call function()
```

#### 与 hrtimer 对比（嵌入式/HFT 必知）

| 特性 | `timer_list` | **`hrtimer`** |
|------|--------------|---------------|
| 分辨率 | **≥ 1 jiffy**（受 HZ 限） | **纳秒级**（硬件 clockevent） |
| 队列 | 链表 | **红黑树** |
| 典型用途 | 老驱动、秒级 housekeeping | **高精度超时**、**itimers**、**NO_HZ** 唤醒 |

#### 常见坑

| 坑 | 对策 |
|----|------|
| **`del_timer` 后回调仍跑** | 用 **`del_timer_sync`** 或在回调里检查 **shutdown 标志** |
| **在回调里 `mod_timer` 自己** | 允许 — **周期 timer** 常用 |
| **持 spinlock 调 `del_timer_sync`** | **死锁** — 回调若需同一把锁 |
| **expires 用绝对 jiffies** | `jiffies + timeout` — 非「剩余毫秒」 |

**HFT：** 内核 **动态 timer** 多用于 **驱动/网络栈 housekeeping**（如 ARP 老化）。**亚毫秒** 策略逻辑在 **用户态** — `timerfd` + **`CLOCK_MONOTONIC`** 或 **busy spin**。勿在 **IRQ 上下文** 指望 `mod_timer` 替代 **`hrtimer`** 做 **微秒级** 唤醒。

→ [Ch 8 softirq / tasklet](../../chapter-08-bottom-halves/) · [Ch 11.7 延迟执行](./section-11.7-延迟执行.md) · [07 TLPI 定时器](../../../03-linux-userspace-api/)

### 常见陷阱

1. 混淆 timer_list（低精度）和 hrtimer（高精度）——现代内核优先 hrtimer
2. 以为定时器回调在中断上下文执行——timer_list 在 softirq 上下文，hrtimer 在 hard IRQ 或 softirq
3. 在定时器回调中睡眠——timer_list 回调在 softirq 上下文不能睡眠，hrtimer 也不能

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** timer_list 和 hrtimer 的区别？现代内核推荐用哪个？

<details><summary>答案</summary>

timer_list：低精度（ms 级，基于 jiffies/tick），回调在 TIMER_SOFTIRQ 上下文。hrtimer：高精度（ns 级，基于 hrtimer 框架 + clock_event_device），回调在 hard IRQ 或 HRTIMER_SOFTIRQ 上下文。现代内核推荐 hrtimer——精度更高，且 NO_HZ 模式下 timer_list 也被 hrtimer 模拟。新代码应始终用 hrtimer。

</details>

**Q2.** hrtimer 回调函数为什么不能睡眠？

<details><summary>答案</summary>

hrtimer 回调在 hard IRQ 或 softirq 上下文执行，无 task_struct、不可调度。睡眠需要 schedule()，但 preempt_count != 0 时 schedule() 会 panic。如果需要在定时器回调中做可睡眠操作：① hrtimer 回调返回 HRTIMER_RESTART → 在 softirq 中重新调度 → workqueue 处理。② 或用 delayed_work（基于 timer_list + workqueue）。

</details>

**Q3.** HFT 中定时器的使用场景和替代方案？

<details><summary>答案</summary>

场景：超时检测、心跳发送、定期采样。替代方案：① 用户态用 `timerfd_create()` + `epoll`（可合并到事件循环）。② 轮询模式（DPDK）：不用定时器，主循环中检查 TSC。③ `SCHED_FIFO` 线程 + `clock_nanosleep()`：精确睡眠（但仍有调度延迟）。④ 自旋等待 + RDTSC：最精确但浪费 CPU。HFT 热路径用 ③/④，非热路径用 ①。

</details>

</details>


> ↔ [ULK Ch6 §5 软件定时器与延迟函数](../../../16-linux-kernel-deep/chapter-06-timing/notes/section-5-软件定时器与延迟函数.md)
---
