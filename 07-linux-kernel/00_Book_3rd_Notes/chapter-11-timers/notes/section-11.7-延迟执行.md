## ⑦ 延迟执行 · Delaying Execution

除 **定时器回调**，驱动常需 **「等这么久再继续」**。

#### 忙等待 · Busy Looping

| 做法 | 不断读 **`jiffies`** 直到经过 N 个 tick |
|------|----------------------------------------|
| 缺点 | **浪费 CPU** |
| 适用 | 延迟 **恰好是 tick 整数倍** 且无更好选择 |
| 改善 | 循环内 **`cond_resched()`** — 主动让出（仍不优雅） |

#### 短延迟 · `udelay` / `ndelay` / `mdelay`

| API | 量级 |
|-----|------|
| **`udelay()`** | 微秒级 |
| **`ndelay()`** | 纳秒级（极短） |
| **`mdelay()`** | 毫秒级 |

| 实现 | 启动时按 **BogoMIPS** 校准的 **紧凑忙等循环** |
|------|-----------------------------------------------|
| 特点 | **硬件级忙等** — 不睡眠、占 CPU |

**HFT：** 用户态 **自旋等** 类似；微秒级硬件复位常用 `udelay` 思维，但 **热路径避免**。

#### `schedule_timeout()`

| 属性 | 说明 |
|------|------|
| 行为 | 当前任务 **可中断睡眠** + 内核设定时器 → **至少 N 个 tick** 后唤醒 |
| 优点 | **不空转 CPU** — **最理想** 的较长延迟 |
| 前提 | **进程上下文** · **不能持 spinlock** |

```c
set_current_state(TASK_INTERRUPTIBLE);
schedule_timeout(HZ / 2);   /* 约 0.5 秒 @ HZ=1000 */
```

| 对比 | 上下文 |
|------|--------|
| `udelay` | 任意？忙等 — 中断里也可用（慎用） |
| `schedule_timeout` | **仅进程上下文** |

#### 选型速查

| 需求 | 首选 | 避免 |
|------|------|------|
| **< 几 ms、硬件握手** | `udelay` / `ndelay` | `schedule_timeout(0)` |
| **毫秒～秒、可睡眠** | `schedule_timeout` / **`msleep`** | 忙等 `jiffies` |
| **精确到 ns、可编程** | **`hrtimer`**（内核）/ **`timerfd`**（用户） | 仅靠 HZ |
| **进程上下文长睡** | **`wait_event_*`** + timeout | 持 spinlock 睡 |

**HFT 用户态镜像：** **`pthread_cond_timedwait`** ≈ `schedule_timeout`；**自旋等队列非空** ≈ `udelay` 思维 — 热路径 **预分配 + 无等待** 优于一切 delay API。

→ [Ch 11.6 动态定时器](./section-11.6-动态定时器.md) · [Ch 4 睡眠](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md)

### 常见陷阱

1. 混淆 udelay() 和 msleep()——前者忙等（spin，精确但浪费 CPU），后者睡眠（释放 CPU 但有调度延迟）
2. 在持锁时用 msleep()——spinlock 持有时不能睡眠，mutex 可以
3. 在 HFT 热路径用 sleep/wait——热路径应预分配 + 无等待，delay API 都是后备

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** udelay() / mdelay() / msleep() / schedule_timeout() 的区别？

<details><summary>答案</summary>

udelay(us)：忙等（spin），基于 BogoMIPS 校准的循环，精度 ns 级，不释放 CPU。mdelay(ms)：udelay 的毫秒版。msleep(ms)：睡眠（schedule_timeout + TASK_UNINTERRUPTIBLE），释放 CPU，精度 ms 级 + 调度延迟。schedule_timeout(timeout)：可设置 TASK_INTERRUPTIBLE/UNINTERRUPTIBLE，最灵活。选择：<10us → udelay；>10us 且可睡眠 → msleep/schedule_timeout。

</details>

**Q2.** 为什么 spinlock 持有时只能 udelay 不能 msleep？

<details><summary>答案</summary>

Spinlock 持有时 preempt_count > 0（或中断禁用）。msleep() 内部调 schedule()，schedule() 检查 preempt_count == 0 才允许调度。违反 → BUG: scheduling while atomic → panic。mutex 持有时 preempt_count == 0（mutex 不禁抢占），可以 msleep()。但如果 mutex 持有时睡眠，其他等待者被阻塞。

</details>

**Q3.** HFT 用户态如何做精确延迟？

<details><summary>答案</summary>

```c
// 方法 1: 自旋 + RDTSC（最精确，浪费 CPU）
uint64_t target = rdtsc() + ns * tsc_ghz;
while (rdtsc() < target) _mm_pause();  // PAUSE 指令降功耗
// 方法 2: nanosleep（释放 CPU，有调度延迟）
struct timespec ts = { .tv_sec = 0, .tv_nsec = ns };
nanosleep(&ts, NULL);  // 最小 ~50us（调度开销）
// 方法 3: futex  spin（自适应）
// HFT 热路径: 方法 1（自旋）, <1us 精确
// HFT 非热路径: 方法 2, 节省 CPU
```

</details>

</details>

---
