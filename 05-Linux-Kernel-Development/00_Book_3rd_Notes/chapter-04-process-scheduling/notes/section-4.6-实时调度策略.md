## ⑥ 实时调度策略 · Real-Time

RT 进程由 **独立实时调度器** 管理 — **不走 CFS 红黑树**。

| 策略 | 行为 |
|------|------|
| **`SCHED_FIFO`** | 先进先出 — **无时间片**；同优先级先运行直到 **阻塞或 yield** |
| **`SCHED_RR`** | 轮转 — 同优先级带 **时间片**，用完排回队尾 |

#### 软实时 · Soft Real-time

| 承诺 | 说明 |
|------|------|
| Linux **尽力** 在期限内调度 RT 任务 | |
| **无硬性绝对保证** | 不同于硬实时 OS |

**HFT 实盘：** 关键线程常用 **`SCHED_FIFO` + 高 RT 优先级 + `sched_setaffinity`**；慎滥用 — 饿死 CFS 线程会导致 **系统管理/日志/网卡慢路径** 失灵。

→ [08-TLPI Ch 34–37](../../../../08-The-Linux-Programming-Interface/) · [03 架构课 a09 调度](../../../05-Linux-Kernel-Development/03_Course_Kernel_Architecture/CHECKLIST.md)

---
