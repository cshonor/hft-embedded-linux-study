## ⑤ 实际时间 / 墙上时间 · Time of Day

**墙上时间** = 人类日历意义上的「现在」 — 与 **单调 tick**（`jiffies`）分离管理。

#### 核心数据

| 变量 / 结构 | 含义 |
|-------------|------|
| **`xtime`** | `struct timespec` — **Epoch（1970-01-01 UTC）** 起的 **秒 + 纳秒** |
| **`wall_to_monotonic`** | 墙上与 **`CLOCK_MONOTONIC`** 的偏移（概念上） |
| **`timezone` / `sys_tz`** | 用户可见 **时区**（现代多用 **TZ 环境变量**） |

```
RTC（boot）──► xtime 初值
tick / NTP  ──► 持续微调 xtime
                │
                ├──► gettimeofday / clock_gettime(REALTIME)
                └──► 日志时间戳、行情 UTC 对齐
```

#### 并发保护 · seqlock

| 机制 | 说明 |
|------|------|
| **`xtime_lock`（seqlock）** | **读多写少** — 读者 **无锁重试**；写者 **短临界区** |
| 读者模式 | 读 **序列号** → 读 `xtime` → 序列号未变则成功 |
| 与 Ch 10 | **seqlock 典型教材例** — 别在读者里睡眠 |

| 写者来源 | 场景 |
|----------|------|
| **tick 路径** | 粗粒度 **+1/HZ** |
| **NTP / adjtimex** | **slewing**（渐调）或 **step**（跳变） |
| **settimeofday** | 管理员 **硬设** 时钟 |

#### 用户空间 API 映射

| 用户 API | 内核 / 行为 |
|----------|-------------|
| **`gettimeofday()`** | **`sys_gettimeofday()`** — 读 `xtime` + tz |
| **`time()`** | 秒级 Epoch |
| **`clock_gettime(CLOCK_REALTIME)`** | 纳秒级墙上 |
| **`clock_gettime(CLOCK_MONOTONIC)`** | **不受 NTP 回拨** — **测间隔首选** |
| **`clock_gettime(CLOCK_MONOTONIC_RAW)`** | 连 **NTP slewing** 也绕开 — **HFT  profiling** 常用 |

#### NTP 与 HFT 实务

| 现象 | 风险 |
|------|------|
| **NTP step 回拨** | `CLOCK_REALTIME` **倒退** → 订单 ID、日志乱序 |
| **slewing** | REALTIME **变慢/变快** — 跨机 **事件对齐** 仍可用 PTP |
| **PTP（IEEE 1588）** | 网卡 **硬件时间戳** + **`phc2sys`** — 交易所机房常见 |

| 实践 | 建议 |
|------|------|
| **延迟测量** | **`MONOTONIC_RAW` / TSC** |
| **合规时间戳** | **REALTIME + PTP**，接受 **adjtimex** 管理 |
| **`CLOCK_TAI`** | 闰秒语义 — 部分 venue 要求 |

**HFT：** 内核 `xtime` 让你懂 **系统时间从哪来**；实盘代码 **几乎总在用户态** 调 `clock_gettime`。驱动/内核模块若打 **UTC 日志**，要知道 **NTP step** 可能发生 — 关键路径用 **单调时钟** 做 **timeout**。

→ [Ch 10 seqlock](../../chapter-10-kernel-synchronization/) · [07 TLPI 时间章](../../../../07-The-Linux-Programming-Interface/) · [01 CSAPP 无时钟章但见并发](../../../../01-CSAPP-3rd/)

---
