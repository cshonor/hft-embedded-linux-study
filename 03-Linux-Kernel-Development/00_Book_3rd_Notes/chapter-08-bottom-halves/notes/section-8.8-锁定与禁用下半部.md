## ⑧ 锁定与禁用下半部

下半部在 **中断返回后异步** 执行 — 与 ISR、进程上下文 **共享数据** 时必须加锁。

#### `local_bh_disable()` / `local_bh_enable()`

| API | 作用 |
|-----|------|
| **`local_bh_disable()`** | **本 CPU** 禁止 **softirq + tasklet** 处理 |
| **`local_bh_enable()`** | 重新启用 |

| 注意 | 说明 |
|------|------|
| **不包括 workqueue** | worker 是进程上下文，用 **mutex** 等 |
| 常与 **自旋锁** 配合 | 防 **死锁**（持锁时若被下半部抢同锁） |

```c
spin_lock_irqsave(&lock, flags);   /* 常同时关中断 + 关 bh */
/* 临界区 — ISR 与 softirq/tasklet 不会穿插 */
spin_unlock_irqrestore(&lock, flags);
```

→ **Ch 9–10** 内核同步详解

---
