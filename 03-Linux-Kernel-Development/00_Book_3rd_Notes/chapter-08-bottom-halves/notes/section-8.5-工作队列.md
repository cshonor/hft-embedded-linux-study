## ⑤ 工作队列 · Work Queues

| 属性 | 说明 |
|------|------|
| 执行者 | **工作者线程（worker threads）** |
| 上下文 | **进程上下文** — 有 `current` |
| 能力 | **唯一允许阻塞/睡眠的下半部** |

| 可做 | 示例 |
|------|------|
| 睡眠 | `mutex_lock`、等信号量 |
| 大块分配 / 块 I/O | 可能触发回收、等磁盘 |

#### 默认队列

| 队列 | 说明 |
|------|------|
| **`events/n`** | 每 CPU 默认 **通用** worker — 驱动不必自建线程 |

```c
schedule_work(&work);           /* 或 queue_work() */
/* 在 events/n 线程里跑 work.func */
```

→ **Ch 6** `kfifo` 中断入队 + workqueue 出队模式

---
