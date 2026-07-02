## ⑤ 实际时间 / 墙上时间 · Time of Day

| 变量 | 类型 | 含义 |
|------|------|------|
| **`xtime`** | `struct timespec` | 自 **1970-01-01 Epoch** 起的 **秒 + 纳秒** |

#### 并发保护

| 机制 | 说明 |
|------|------|
| **`xtime_lock`（seqlock）** | 读多写少 — **Ch 10 seqlock** |

#### 用户空间

| API | 内核实现 |
|-----|----------|
| **`gettimeofday()`** | **`sys_gettimeofday()`** |

**HFT：** 行情 **UTC 对齐**、日志时间戳 — 用户态更常用 **`clock_gettime(CLOCK_REALTIME/MONOTONIC)`**；懂 `xtime` 即懂 **系统时间从哪来**。

→ [06-The-Linux-Programming-Interface 时间章](../../../../06-The-Linux-Programming-Interface/)

---
