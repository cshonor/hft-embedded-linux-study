## ④ 休眠与唤醒 · Sleeping and Waking Up

进程等事件（磁盘 I/O、键盘…）时：

| 步骤 | 动作 |
|------|------|
| 1 | 标为 **`TASK_INTERRUPTIBLE`** 或 **`TASK_UNINTERRUPTIBLE`** |
| 2 | 从 CFS **红黑树移除** |
| 3 | 挂到 **等待队列（Wait Queue）** |

事件就绪时：

| 步骤 | 动作 |
|------|------|
| 1 | **`wake_up()`**（及变体）遍历等待队列 |
| 2 | 状态 → **`TASK_RUNNING`** |
| 3 | **重新插入红黑树** — 参与 CFS 竞争 |

```
RUNNING ──睡眠──► 移出红黑树 ──► wait queue
                                    │
              wake_up ◄── 事件完成 ──┘
                 │
                 └──► RUNNING + 插回红黑树
```

→ **Ch 3** 五态 · **Ch 9–10** 等待队列与锁

---
