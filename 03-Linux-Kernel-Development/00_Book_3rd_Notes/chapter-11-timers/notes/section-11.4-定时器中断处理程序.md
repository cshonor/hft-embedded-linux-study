## ④ 定时器中断处理程序

分为 **体系结构相关** 入口 + **体系结构无关** 核心逻辑。

#### `tick_periodic()`（概念职责）

每次 tick 大致做：

| 工作 | 说明 |
|------|------|
| **`jiffies_64++`** | 全局节拍推进 |
| **进程资源统计** | 当前进程 CPU 时间等 |
| **到期动态定时器** | 检查并触发 |
| **`scheduler_tick()`** | 时间片、CFS、**可能触发抢占** |
| **更新 `xtime`** | 墙上时间推进 |
| **负载计算** | 系统 load average 等 |

```
timer IRQ
    ▼
tick_periodic()
    ├─ jiffies_64++
    ├─ run timers
    ├─ scheduler_tick()  ──► Ch 4
    └─ update xtime (seqlock)
```

**HFT：** `scheduler_tick` 出现在 **非绑核 / 非 FIFO** 路径上 → 理解 **tick 抖动** 来源之一。

---
