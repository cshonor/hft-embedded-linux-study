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

---
