## ② jiffies 变量

`jiffies` 是内核的 **「打了多少个 tick」** 计数器 — 绝大多数 **相对超时** 都建立在它之上。

#### 核心变量

| 变量 | 说明 |
|------|------|
| **`jiffies`** | 对外常用 **unsigned long**；Historically 多为 **低 32 位** 视图 |
| **`jiffies_64`** | **64 位** 真值 — 在 32 位内核上防溢出 |
| **更新点** | 每次 timer IRQ 路径 **`jiffies_64++`**（Ch 11.4） |

```c
/* 典型超时写法 */
unsigned long timeout = jiffies + msecs_to_jiffies(500);  /* 约 500ms 后 */
while (!done) {
    if (time_after(jiffies, timeout))
        break;
    /* ... */
}
```

#### 溢出与回绕 · Wrap-around

| 配置 | 回绕时间（量级） |
|------|------------------|
| 32 位 `jiffies` @ HZ=1000 | **~49.7 天** |
| 32 位 @ HZ=100 | **~497 天** |
| 64 位 `jiffies_64` |  practically 永不担心 |

**错误写法：** `if (jiffies > expires)` — 回绕后会 **误判已过期/未过期**。

| 宏 | 语义 |
|----|------|
| **`time_after(a, b)`** | a 是否在 b **之后**（回绕安全） |
| **`time_before(a, b)`** | a 是否在 b **之前** |
| **`time_after_eq` / `time_before_eq`** | 含相等边界 |
| **`time_in_range(j, start, end)`** | j 是否在 [start, end) 内 |

原理：把差值当作 **有符号** 比较 — 只要超时窗口 **小于 jiffies 半圈** 就安全。

#### 时间单位换算

| 宏 / API | 方向 |
|----------|------|
| **`msecs_to_jiffies(ms)`** | 毫秒 → jiffies |
| **`jiffies_to_msecs(j)`** | jiffies → 毫秒 |
| **`usecs_to_jiffies` / `jiffies_to_usecs`** | 微秒档（仍受 HZ 下限约束） |
| **`HZ`** | 1 秒 = **HZ** 个 jiffies |

| 事实 | 含义 |
|------|------|
| `schedule_timeout(1)` | **至少** 1 tick，不是精确 1/HZ 秒 |
| HZ=1000 时 1 jiffy | ~1 ms；HZ=100 时 ~10 ms |

#### `USER_HZ` 与用户空间

向用户导出 `/proc/stat`、`times()` 等时，内核用 **`USER_HZ`**（常为 **100**）做 **jiffies ↔ 用户态 tick** 的稳定换算 — 与用户 **`sysconf(_SC_CLK_TCK)`** 一致，避免内核改 HZ 后用户工具失真。

#### 并发与读取

| 机制 | 说明 |
|------|------|
| **`jiffies` 读取** | 单写多读；32 位上读 64 位值可能 **撕裂** — 用 **`jiffies_64` + seqlock** 或 **`get_jiffies_64()`** |
| 写 | **仅** timer IRQ 核心路径递增 — 不与其他写者竞争 |

**HFT：** 内核模块里用 `jiffies` 做 **秒级 watchdog** 可以；**亚毫秒 SLA** 请用 **`ktime_get()` / hrtimer**。用户态对应 **`CLOCK_MONOTONIC`**，不要用 `jiffies` 思维去量 **纳秒级** 行情延迟。

→ [Ch 10 seqlock](../../chapter-10-kernel-synchronization/) · [Gorman Ch3 时间](../../../../06-linux-mm/chapter-03-page-table-management/)（页表章亦涉及时序）

### 常见陷阱

1. 混淆 jiffies 和 jiffies_64——jiffies 是 32 位（可能回绕），jiffies_64 是 64 位
2. 用 jiffies 做精确计时——jiffies 精度 = 1/HZ（1ms 或 10ms），不适合纳秒级计时
3. 忽略 jiffies 回绕——32 位 jiffies 在 HZ=1000 时 ~49 天回绕，用 time_after() 比较

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** jiffies 回绕是什么问题？怎么安全比较时间？

<details><summary>答案</summary>

jiffies 是 unsigned long（32 位），HZ=1000 时 ~49.7 天回绕（2^32 / 1000 / 86400 ≈ 49.7）。直接比较 `jiffies > deadline` 在回绕时出错。安全比较：`time_after(jiffies, deadline)` 和 `time_before(jiffies, deadline)`——内部用有符号差值处理回绕。`time_in_range(jiffies, start, end)` 检查是否在范围内。

</details>

**Q2.** 为什么 jiffies 不适合 HFT 计时？

<details><summary>答案</summary>

① 精度低：HZ=1000 时只有 1ms 精度，HFT 需要纳秒级。② 不是单调的：wall time 可被 NTP 调整（跳变）。③ 32 位回绕风险。HFT 应使用：① `ktime_get()` / `ktime_get_ns()`：纳秒级，单调。② `rdtsc()` / `__rdtsc()`：CPU TSC，纳秒级，最快（~20ns）。③ `clock_gettime(CLOCK_MONOTONIC)`：走 vDSO，~20ns。

</details>

**Q3.** HFT 如何用 TSC 做精确计时？

<details><summary>答案</summary>

```c
#include <x86intrin.h>
uint64_t t1 = __rdtsc();  // 读 TSC
// ... 待测代码 ...
uint64_t t2 = __rdtsc();
double ns = (double)(t2 - t1) / tsc_frequency_ghz;
// 获取 TSC 频率: cat /proc/cpuinfo | grep MHz
// 注意: 1) TSC 在现代 CPU 上是不变的(invariant)
//       2) 多核间 TSC 同步(但老 CPU 可能不同步)
//       3) 用 RDTSCP 替代 RDTSC 保证顺序
```

</details>

</details>

---
