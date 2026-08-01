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

→ [Ch 8 softirq / tasklet](../../chapter-08-bottom-halves/) · [Ch 11.7 延迟执行](./section-11.7-延迟执行.md) · [07 TLPI 定时器](../../../../04-linux-userspace-api/)

---
