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

→ [Ch 10 seqlock](../../chapter-06-kernel-data-structures) · [07 TLPI 时间章](../../../03-linux-userspace-api/) · [01 CSAPP 无时钟章但见并发](../../../02-computer-systems/)

### 常见陷阱

1. 混淆 CLOCK_REALTIME 和 CLOCK_MONOTONIC——前者可被 NTP 调整（会跳变），后者不会
2. 用 CLOCK_REALTIME 做计时——NTP 调整可能导致时间倒流，计时段为负
3. 在内核中用 do_gettimeofday()——已废弃，应用 ktime_get_real_ts() / ktime_get()

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** CLOCK_REALTIME / CLOCK_MONOTONIC / CLOCK_MONOTONIC_RAW 的区别？

<details><summary>答案</summary>

REALTIME：墙上时间（1970-01-01 起的秒数），可被 NTP/settimeofday 调整，可能跳变。MONOTONIC：单调递增，不受 NTP 调整影响（但受 NTP 频率调整影响，可能快慢漂移）。MONOTONIC_RAW：纯硬件时钟，完全不受 NTP 影响。HFT 用 MONOTONIC（单调 + 不跳变）。`clock_gettime(CLOCK_MONOTONIC, &ts)` 走 vDSO（~20ns）。

</details>

**Q2.** 为什么 HFT 不用 CLOCK_REALTIME？

<details><summary>答案</summary>

① NTP 调整可能导致时间跳变（向前或向后）→ 计时段为负或异常大。② `settimeofday()` 可被 root 手动设置 → 不可预测。③ 跨机器时间同步需要 NTP/PTP，但同步应通过 PTP 硬件时间戳在应用层处理，不依赖系统时钟。HFT 用 MONOTONIC 做本地计时，用 PTP 做跨机器同步。

</details>

**Q3.** HFT 如何获取纳秒级单调时间？

<details><summary>答案</summary>

```c
#include <time.h>
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);
// 走 vDSO, ~20ns, 不进内核
// 或直接 RDTSC:
uint64_t tsc = __rdtsc();
uint64_t ns = tsc * 1000 / tsc_khz;
// 需要校准 TSC 频率: tsc_khz = 基准频率 * 1000
// HFT 最佳: RDTSC + 预校准频率, 延迟 < 50ns
```

</details>

</details>

---
