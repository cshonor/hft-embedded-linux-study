## ③ 进程状态 · Process State

`task_struct` 的 **`state`** 字段 — 任一时刻必居 **下列状态之一**（经典 3rd 表述；新内核用位掩码组合，直觉相同）：

| 状态 | 宏 | 含义 |
|------|-----|------|
| **运行** | `TASK_RUNNING` | 正在 CPU 上跑，或在 **运行队列** 里等 CPU |
| **可中断睡眠** | `TASK_INTERRUPTIBLE` | 阻塞等事件；**收到信号可提前唤醒** → 可回到运行 |
| **不可中断睡眠** | `TASK_UNINTERRUPTIBLE` | 阻塞等事件；**信号不能唤醒** — 常用于必须等完的 I/O |
| **被跟踪** | `__TASK_TRACED` | 被调试器 **`ptrace`** 跟踪 |
| **停止** | `__TASK_STOPPED` | 收到 **`SIGSTOP` / `SIGTSTP`** 等而暂停 |
| **退出僵尸** | `EXIT_ZOMBIE` | 已 `exit`，等父 `wait` — [§3.6](./section-3.6-进程终结.md) |

#### ps STAT 列对照

| ps 字符 | 内核态 |
|---------|--------|
| **R** | `TASK_RUNNING` |
| **S** | 可中断睡眠 |
| **D** | 不可中断睡眠 |
| **T/t** | 停止 / 跟踪停止 |
| **Z** | 僵尸 |

#### 状态迁移（简化）

```
        调度选中                    wake_up / 信号
  RUNNING ◄───────────────────────────────┐
     │                                      │
     │ schedule()                           │
     │ 等待资源/睡眠                         │
     ▼                                      │
 INTERRUPTIBLE / UNINTERRUPTIBLE ────────────┘
     │
     │ SIGSTOP / ptrace
     ▼
 TRACED / STOPPED
```

#### 谁改 state？

| 路径 | 典型调用链 |
|------|------------|
| **主动让出 CPU** | `schedule()` ← mutex、wait_queue |
| **时间片耗尽** | 时钟中断 → 调度器 — [§4.5](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) |
| **唤醒** | `try_to_wake_up()` → 入运行队列 |
| **退出** | `do_exit()` → `EXIT_ZOMBIE` |

#### 可中断 vs 不可中断

| 类型 | 信号能打断？ | 典型场景 | 风险 |
|------|-------------|----------|------|
| **可中断** | 是 | 等 socket、等 futex | 需处理 `-EINTR` |
| **不可中断** | 否 | 部分块设备、NFS 卡住 | **`D` 状态堆积** |

```c
/* 驱动里睡眠（示意） */
set_current_state(TASK_INTERRUPTIBLE);
schedule();          /* 不再返回直到被唤醒 */
set_current_state(TASK_RUNNING);
```

#### 与调度器的边界

| 状态 | 是否在 CFS/RT 运行队列？ |
|------|--------------------------|
| **RUNNING** | 是（含「就绪未跑」） |
| **睡眠 / 停止 / 僵尸** | 否 |

**HFT / 观测：** `D` 状态（不可中断睡眠）过多 → 磁盘/NFS 等阻塞拖慢整条流水线；`perf sched latency`、`ps aux` 与 **Ch 4 运行队列** 联读。用户态热路径若频繁 `futex` 睡眠，STAT 会长期在 **S** — 唤醒延迟是尾延迟来源之一。

→ [Ch 4 §4.4 休眠与唤醒](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md) · [Ch 4 §4.5 抢占](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [15 SysPerf §3.2 进程与调度](../../../06.6-systems-performance/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [07 TLPI Ch29–33 线程与调度](../../../03-linux-userspace-api/chapter-33-threads-further/notes)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的区别？哪个对 HFT 更关键？

<details><summary>答案</summary>

TASK_INTERRUPTIBLE = 可被信号唤醒的睡眠（如 read 等数据）；TASK_UNINTERRUPTIBLE = 不可被信号唤醒的睡眠（如等待磁盘 IO，只能等 IO 完成）。HFT 关注 TASK_INTERRUPTIBLE：交易线程 poll 时如果被信号打断会丢数据，需要 SA_RESTART 或 busy-poll。

</details>

**Q2.** 进程处于 TASK_RUNNING 但没在 CPU 上运行是什么意思？

<details><summary>答案</summary>

TASK_RUNNING 表示「可运行」（在运行队列中等待调度），不一定是「正在运行」。可能多个进程都是 TASK_RUNNING 但只有一个在 CPU 上。CFS 调度器从运行队列中选 vruntime 最小的运行。HFT 绑核 + SCHED_FIFO 就是为了让交易线程永远在 CPU 上，不进运行队列等待。

</details>

</details>
---
