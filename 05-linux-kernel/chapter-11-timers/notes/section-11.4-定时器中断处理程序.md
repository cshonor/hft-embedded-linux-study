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

→ [Ch 4 抢占与 tick](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [Ch 8 softirq](../../chapter-08-bottom-halves/) · [Ch 10 seqlock](../../chapter-06-kernel-data-structures)

### 常见陷阱

1. 把 ULK 的定时器中断处理当现代版——6.x 用 hrtimer + NO_HZ + tickless 机制完全不同
2. 混淆 scheduler_tick() 和 timer tick——scheduler_tick() 是 tick 中断的一部分，更新调度统计
3. 以为定时器中断频率固定——NO_HZ 下可以动态调整或完全停止

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 现代内核的定时器中断处理和 ULK 时代有什么区别？

<details><summary>答案</summary>

ULK 时代：固定 HZ 频率的 tick → `do_timer_interrupt()` → 更新 jiffies + scheduler_tick() + 检查 timer list。现代：① hrtimer 框架取代 timer list（精度从 ms 提升到 ns）。② NO_HZ：idle 时停止 tick。③ nohz_full：single-task 时停止 tick。④ tickless：动态计算下一个需要的 tick 时间。⑤ `tick_nohz_idle_enter()` / `tick_nohz_idle_exit()`。

</details>

**Q2.** scheduler_tick() 做什么？对 HFT 有什么影响？

<details><summary>答案</summary>

① 更新当前进程的 vruntime / 时间统计。② 检查时间片是否耗尽 → 设 need_resched。③ 更新 CPU 负载统计。④ 触发 RT 负载均衡检查。影响：每次 tick 中断交易线程 ~1-5us。HFT 用 `nohz_full` 消除交易核的 tick → scheduler_tick() 不执行 → 交易线程不被中断。

</details>

**Q3.** HFT 如何配置 tickless 减少定时器中断？

<details><summary>答案</summary>

```bash
# 内核启动参数
isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3
# 确认
cat /sys/devices/system/cpu/nohz_full
# 应输出 2-3
cat /proc/interrupts | grep LOC
# 2-3 号 CPU 的 LOC（local timer）计数应几乎不变
# 注意: nohz_full CPU 上只能跑一个任务(或 RT 线程组)
```

</details>

</details>


> ↔ [ULK Ch6 §4 更新时间与统计](../../../16-linux-kernel-deep/chapter-06-timing/notes/section-4-更新时间与统计.md)
---
