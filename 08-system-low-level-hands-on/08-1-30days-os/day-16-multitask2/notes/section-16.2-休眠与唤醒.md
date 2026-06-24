## ② 休眠与唤醒 · 消除算力浪费

Day 15 **固定轮转** → 任务 A（`HariMain`）**等键鼠** 时仍 **每 0.02s 切到 A**，A **空转** 浪费。

#### Sleep

| API（示意） | 行为 |
|-------------|------|
| **`task_sleep(task)`** | 从 **`running` 活动列表** 移除 → **不参与** 时间片轮转 |

#### Wake

| 触发 | 行为 |
|------|------|
| 键/鼠 ISR → **FIFO Put** | 改 **`fifo.c`**：有数据时 **`task_wake(任务A)`** |
| **`task_wake`** | 重新 **加入 running** |

```
A 等输入 → task_sleep(A)     B0~B2 全速计数
键/鼠事件 → fifo → wake(A)   A 立刻回到候选队列
```

**CPU 只分给「有事做」的任务** — **idle 不占片**。

**HFT：** **阻塞在 epoll/eventfd** vs **busy spin** — OS 层 **sleep 等事件** 是正道；HFT 热路径 **反过来的：少 block、少切换**。

→ [Day 13 统一 FIFO](../day-13-timer2/) · [Day 7 ISR 短、主循环处理](../day-07-fifo-mouse/)

---
