## ④ 休眠与唤醒 · Sleeping and Waking

任务不一定一直可运行：等 I/O、等锁、等事件时进入 **睡眠**，事件到达后 **唤醒** 再回运行队列。

#### 状态直觉（与 Ch 3 呼应）

| 状态 | 含义 | 调度器可见？ |
|------|------|--------------|
| **TASK_RUNNING** | 可运行（含正在跑） | 在运行队列 / 树上 |
| **可中断睡眠** | 等事件，可被信号打断 | **不在** 可运行集合 |
| **不可中断睡眠** | 等事件，信号也难打断（如部分磁盘路径） | 同上 |
| **TASK_DEAD 等** | 退出路径 | — |

#### 等待队列 · Wait Queue

| 概念 | 作用 |
|------|------|
| **`wait_queue_head_t`** | 某事件上「谁在等」的链表头 |
| **睡眠方** | 把自己挂到队列 → 设状态 → `schedule()` 让出 CPU |
| **唤醒方** | `wake_up()` / `wake_up_interruptible()` → 任务重回运行队列 |

```
任务 A 需要事件 E
  │
  ├─ 把 A 加入 E 的 wait queue
  ├─ 设 TASK_*SLEEP*
  └─ schedule() ──► CPU 去跑别人
         │
事件 E 发生（中断/另一任务）
  │
  └─ wake_up() ──► A 变可运行 ──► 进 CFS/RT 队列 ──► 稍后真正上 CPU
```

#### 虚假唤醒 · Spurious Wakeup

| 规则 | 做法 |
|------|------|
| 醒来后 **必须再检查条件** | `while (!condition) sleep_again;` |
| 原因 | 可能多人等同一队列、条件已变、被信号打断 |

#### 睡眠禁忌（驱动 / HFT 都要命）

| 禁止 | 原因 |
|------|------|
| **持 spinlock 时睡眠** | 自旋等待者空转；易死锁 |
| **中断上下文睡眠** | 无「当前进程」可换下 |
| **原子上下文（atomic）里睡眠** | 同 ISR / softirq 规则 |

**HFT：** 用户态 `futex` / 条件变量 = 同一「等事件 → 唤醒」故事；热路径若频繁睡眠，尾延迟来自 **调度唤醒延迟**，不是算法本身。

→ [4.5 抢占与切换](./section-4.5-抢占与上下文切换.md) · [Ch 10 mutex/completion](../../chapter-10-sync-methods/)

### 常见陷阱

1. 把「休眠」当浪费 CPU——休眠释放 CPU 给其他任务，是高效的资源利用
2. 混淆 `TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE`——前者可被信号唤醒，后者不可
3. 以为唤醒后立即运行——唤醒只是把进程放回运行队列，是否立即运行取决于调度器

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE` 的区别？

<details><summary>答案</summary>

INTERRUPTIBLE：可被信号唤醒（`kill -9` 有效），进程可响应异步事件。UNINTERRUPTIBLE：不可被信号唤醒（`kill -9` 无效），通常在等磁盘 I/O 等不可中断操作。`TASK_KILLABLE` 是 2.6.25+ 新增：可被致命信号唤醒但不被普通信号打断。HFT 避免在热路径上进入 UNINTERRUPTIBLE（D 状态，无法 kill）。

</details>

**Q2.** 唤醒抢占的阈值是什么？为什么需要？

<details><summary>答案</summary>

`sched_wakeup_granularity`（默认 1ms）。唤醒的进程 vruntime 比 current 小这个阈值时才抢占。太低（0）会导致频繁抢占（thrashing）；太高会导致交互延迟。HFT 用 SCHED_FIFO 不走 CFS，不受此影响。

</details>

**Q3.** HFT 如何避免热路径上的休眠/唤醒延迟？

<details><summary>答案</summary>

① 预分配所有资源（内存/连接/FD），避免运行时等资源。② 无锁队列代替 mutex（mutex 阻塞 = 休眠）。③ DPDK 用户态轮询代替 epoll_wait（epoll_wait 阻塞 = 休眠）。④ `SCHED_FIFO` 确保唤醒后立即运行（RT 优先级 > CFS）。

</details>

</details>

---
