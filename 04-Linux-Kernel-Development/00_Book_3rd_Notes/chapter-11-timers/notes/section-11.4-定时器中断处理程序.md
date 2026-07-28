## ④ 定时器中断处理程序

每次 **系统定时器中断** 是内核的 **心跳** — 架构相关 **入口** 很快跳进 **体系结构无关** 的核心逻辑。

#### 分层结构

```
arch timer IRQ（汇编入口，保存寄存器）
    ▼
do_timer / tick handler（平台相关薄层）
    ▼
tick_periodic() / tick_sched_timer（核心）
    ├─ update_process_times()   /* 当前进程 CPU 时间 */
    ├─ run_local_timers()       /* 到期 timer_list */
    ├─ scheduler_tick()         /* Ch 4 — 可能 need_resched */
    ├─ update_wall_time()       /* xtime 推进 */
    └─ calc_global_load()       /* load average */
```

#### `tick_periodic()` 职责（概念清单）

| 工作 | 说明 |
|------|------|
| **`jiffies_64++`** | 全局 **节拍** 推进 |
| **进程资源统计** | **`user/system time`**、**`profile`** 采样 |
| **到期动态定时器** | **`run_timer_softirq`** 或直接检查链表 |
| **`scheduler_tick()`** | CFS **vruntime**、RR 时间片、**实时 band** — **可能触发抢占** |
| **更新墙上时间** | **`xtime`** 按 tick 累加（精细调整走 NTP/hrtimer 路径） |
| **负载计算** | **1/5/15 分钟 load average** |

#### 中断上下文约束

| 事实 | 影响 |
|------|------|
| 运行在 **hardirq 或 irq-off 临界区** | **不能睡眠** |
| 必须 **短小** | 拖长 → **全系统 tick 延迟** → 调度/grace period 抖动 |
|  heavier work |  defer 到 **softirq / kthread** |

#### 与 hrtimer / tickless 的关系（书外补全）

| 模型 | 行为 |
|------|------|
| **传统 periodic tick** | 每 `1/HZ` 秒进一次上述路径 |
| **NO_HZ idle** | CPU idle 时 **停 tick**，醒来的 **deadline** 由 hrtimer 编程 |
| **NO_HZ_FULL** | 运行用户线程的隔离核 **尽量无 tick** — **scheduler_tick 也少来** |

```
传统：  |--tick--|--tick--|--tick--|--tick--|
NO_HZ： |-------- sleep --------|--tick--|  （仅到点唤醒）
```

#### SMP 注意点

| 点 | 说明 |
|----|------|
| **每 CPU tick** | 各核 **独立** `jiffies` 更新？Historically global；现代常 **global jiffies + per-CPU kstat** |
| **负载均衡** | **`scheduler_tick`** 里可能触发 **周期性迁移检查** — 与 **绑核 HFT** 冲突 → **isolcpus** |

**HFT：** `scheduler_tick` 是 **CFS 线程** 在 **非 FIFO、非 isolcpus** 核上的 **抖动源之一**。实盘：

1. 策略线程 **`SCHED_FIFO` + 绑核**
2. 隔离核 **`nohz_full=...`** 减少 **无关 tick**
3. 用 **`/proc/interrupts`**、**`perf`** 看 **LOC timer** 频率是否仍过高

→ [Ch 4 抢占与 tick](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [Ch 8 softirq](../../chapter-08-bottom-halves/) · [Ch 10 seqlock](../../chapter-10-kernel-synchronization/)

---
