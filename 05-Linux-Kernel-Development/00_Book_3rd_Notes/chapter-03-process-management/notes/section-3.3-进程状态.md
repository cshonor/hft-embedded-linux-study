## ③ 进程状态 · Process State

`task_struct` 的 **`state`** 字段 — 任一时刻必居 **下列五态之一**（经典 3rd 表述）：

| 状态 | 宏 | 含义 |
|------|-----|------|
| **运行** | `TASK_RUNNING` | 正在 CPU 上跑，或在 **运行队列** 里等 CPU |
| **可中断睡眠** | `TASK_INTERRUPTIBLE` | 阻塞等事件；**收到信号可提前唤醒** → 可回到运行 |
| **不可中断睡眠** | `TASK_UNINTERRUPTIBLE` | 阻塞等事件；**信号不能唤醒** — 常用于必须等完的 I/O |
| **被跟踪** | `__TASK_TRACED` | 被调试器 **`ptrace`** 跟踪 |
| **停止** | `__TASK_STOPPED` | 收到 **`SIGSTOP` / `SIGTSTP`** 等而暂停 |

#### 状态迁移（简化）

```
        调度选中
  RUNNING ◄──────────── 就绪队列
     │                      ▲
     │ 等待资源/睡眠          │ 信号/事件就绪
     ▼                      │
 INTERRUPTIBLE / UNINTERRUPTIBLE
     │
     │ ptrace
     ▼
 TRACED / STOPPED
```

**HFT / 观测：** `D` 状态（不可中断睡眠）过多 → 磁盘/NFS 等阻塞拖慢整条流水线；`perf`/`ps` 与 **Ch 4 运行队列** 联读。

→ [03 SysPerf §3.2 进程与调度](../../../../03-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

---
