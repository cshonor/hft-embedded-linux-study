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

→ [Ch 10 seqlock](../../chapter-10-kernel-synchronization/) · [Gorman Ch3 时间](../../../../09-linux-mm/chapter-03-page-table-management/)（页表章亦涉及时序）

---
