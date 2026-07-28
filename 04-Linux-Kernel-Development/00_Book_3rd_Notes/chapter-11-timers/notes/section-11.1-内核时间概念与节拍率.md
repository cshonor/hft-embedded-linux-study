## ① 内核时间概念与节拍率 · HZ

内核不靠「读表」被动得知时间，而靠硬件 **周期性中断** 主动 **推进全局时钟** 并驱动调度、定时器、统计。

#### 两类时间（先建立心智模型）

| 类别 | 内核代表 | 特点 |
|------|----------|------|
| **相对时间 / 单调** | `jiffies`、hrtimer | 自启动递增；**不受 NTP 回拨** 影响 |
| **绝对时间 / 墙上** | `xtime` | 对应 **日历/Epoch**；可被 NTP/adjtime 调整 |

本章前半讲 **节拍（tick）** 如何产生；后半讲 **墙上时间** 与 **延迟执行**。

#### 节拍率 · Tick Rate · `HZ`

| 术语 | 说明 |
|------|------|
| **节拍（Tick）** | 系统定时器触发一次 **时钟中断** |
| **`HZ`** | 编译期宏 — **每秒 tick 次数** |
| **tick 周期** | `1/HZ` 秒 — 内核时间粒度的 **下限之一** |

| x86 常见 HZ | 周期 | 典型场景 |
|-------------|------|----------|
| **100** | 10 ms / tick | 老桌面、部分嵌入式默认 |
| **250** | 4 ms | 中间档 |
| **1000** | 1 ms / tick | 低延迟桌面/服务器常见 |

```
硬件计数器 ──周期溢出──► timer IRQ ──► jiffies++
                                      scheduler_tick()
                                      到期 timer 回调
                                      xtime 推进（粗粒度部分）
```

#### HZ 权衡

| 提高 HZ | 收益 | 代价 |
|---------|------|------|
| ✓ | 基于 tick 的 **超时分辨率** ↑ | **每核每秒 HZ 次中断** → CPU 开销 ↑ |
| ✓ | **时间片/调度** 检查更密 → 平均 **调度延迟** ↓ | **缓存/流水线** 被打断；**功耗** ↑ |
| ✓ | `jiffies` 粒度更细 | 仍 **无法** 替代 **纳秒级 hrtimer** |

#### 无节拍 · Tickless · `NO_HZ`

| 概念 | 说明 |
|------|------|
| **Tickless / NO_HZ** | 空闲或无近期定时事件时 **跳过** 多余 tick |
| **NO_HZ_FULL** | 特定 CPU **几乎不** 收 tick — 专给 **latency-sensitive** 线程 |
| 与 hrtimer | 真正「下一次该醒什么时候」由 **红黑树 hrtimer** 编程硬件，而非盲打 HZ |

| 配置思路 | 目的 |
|----------|------|
| 管理核低 HZ / tickless | 省电、减无关中断 |
| 隔离核 `isolcpus` + `NO_HZ_FULL` | 策略线程 **少被 tick 打断** |

**HFT：** 实盘机常见组合 — **hrtimer/用户态 `clock_gettime(CLOCK_MONOTONIC_RAW)`** 做测量；**绑隔离核** 减 tick 抖动；**勿以为** 把 HZ 拉到 1000 就等于微秒级定时。策略 **deadline** 应用 **hrtimer 或 busy-poll**，不是 `schedule_timeout(1)`。

#### 与调度、同步的接线

| 子系统 | 每 tick 相关动作 |
|--------|------------------|
| **调度（Ch 4）** | `scheduler_tick()` — CFS vruntime、时间片、可能 **need_resched** |
| **定时器（本章）** | 检查 `timer_list` / hrtimer 到期 |
| **统计** | 进程/系统 CPU 时间、load average |

→ [Ch 4 `scheduler_tick`](../../chapter-04-process-scheduling/) · [Ch 10 seqlock](../../chapter-10-kernel-synchronization/) · [07 TLPI 时间章](../../../../07-The-Linux-Programming-Interface/)

---
