## ③ 进程状态 · Process State

`task_struct` 的 **`state`** 字段 — 任一时刻必居 **下列状态之一**（经典 3rd 表述；新内核用位掩码组合，直觉相同）：

```c
/* include/linux/sched.h */
struct task_struct {
        ...
        volatile long state;    /* -1 unrunnable, 0 runnable, >0 stopped */
        ...
};

#define TASK_RUNNING            0x0000
#define TASK_INTERRUPTIBLE      0x0001
#define TASK_UNINTERRUPTIBLE    0x0002
#define __TASK_STOPPED          0x0004
#define __TASK_TRACED           0x0008
/* include/linux/sched.h 末尾（task_state_array 用）+ EXIT_* 在 exit_state 里 */
#define EXIT_DEAD               0x0010
#define EXIT_ZOMBIE             0x0020
```

> 三个看点：`volatile` — state 常被唤醒方在另一个 CPU 上改，禁止编译器缓存到寄存器；RUNNING 的值是 **0** 而非「在跑」标志 —— 它只表示**可运行**，在不在 CPU 上由调度器说了算；EXIT_* 是独立字段 `exit_state`，所以僵尸不算 state 里的一员。

#### state 不是孤军 —— 谁配合它

state 只回答「**能不能跑**」，不回答「**在哪等、等谁**」。后半句由其他字段/结构承担：

| 疑问 | 配套结构 | 方向 |
|------|----------|------|
| 睡着后在哪？等谁？ | **`wait_queue_head_t`** — 属于资源方（设备、futex bucket、epoll 等待项） | **反向**：进程把自己的 `wait_queue_entry`（通常在栈上，`DEFINE_WAIT`）挂到资源的队列里 |
| 就绪后排在哪？ | **`sched_entity`** 挂进 per-CPU 的 cfs_rq / rt_rq | 也不在 task_struct 内 — 调度器视图 [Ch 4](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md) |
| 全局进程清单？ | `list_head tasks` — 串起所有 task_struct | 前向：进程自己持有 |
| 僵尸时等谁收尸？ | `exit_state` + `exit_code` + `real_parent` / `children` | 配合 `wait()` — [§3.6](./section-3.6-进程终结.md) |

```c
/* 睡眠的真实动作：state 改变 + 入队，两者必须配对 */
DEFINE_WAIT(wait);                          /* entry 在本进程内核栈上 */
prepare_to_wait(&wq, &wait, TASK_INTERRUPTIBLE);  /* 挂到资源的等待队列 + 改 state */
schedule();                                 /* 让出 CPU；醒来时从这句之后继续 */
finish_wait(&wq, &wait);                    /* 出队 + state 回 TASK_RUNNING */
```

> 所以 `ps` 里两个都显示 `S` 的进程，可能在等完全不同的东西 —— state 相同、所在的等待队列千差万别。排查「卡在哪」要顺着 fd / wchan（`cat /proc/<pid>/wchan`）找队列，而不是看 state。

| 状态 | 宏 | 含义 |
|------|-----|------|
| **运行** | `TASK_RUNNING` | 正在 CPU 上跑，或在 **运行队列** 里等 CPU |
| **可中断睡眠** | `TASK_INTERRUPTIBLE` | 阻塞等事件；**收到信号可提前唤醒** → 可回到运行 |
| **不可中断睡眠** | `TASK_UNINTERRUPTIBLE` | 阻塞等事件；**信号不能唤醒** — 常用于必须等完的 I/O |
| **被跟踪** | `__TASK_TRACED` | 被调试器 **`ptrace`** 跟踪 |
| **停止** | `__TASK_STOPPED` | 收到 **`SIGSTOP` / `SIGTSTP`** 等而暂停 |
| **退出僵尸** | `EXIT_ZOMBIE` | 已 `exit`，等父 `wait` — [§3.6](./section-3.6-进程终结.md) |

#### ps STAT 列对照

| ps 字符 | 内核态 | 怎么亲眼看到（复现例子） |
|---------|--------|--------------------------|
| **R** | `TASK_RUNNING` | `while :; do :; done &` 死循环 — 一直 R；`ps -o stat= -p $!` |
| **S** | 可中断睡眠 | 几乎所有后台守护进程（sshd、cron）；前台 `sleep 100` — 99% 时间在 S |
| **D** | 不可中断睡眠 | 块设备卡死/NFS 断连时出现：`dd if=/dev/sda of=/dev/null` 期间拔盘；正常机器上转瞬即逝，**长期 D 才是故障** |
| **T** | 停止 | `sleep 300` 后按 **Ctrl+Z**（SIGTSTP）；或 `kill -STOP <pid>`（SIGSTOP 不可捕获） |
| **t** | 跟踪停止 | `gdb ./a.out` 打断点停在断点上时，被调试进程就是 t |
| **Z** | 僵尸 | 父进程生而不收的子进程（下方代码） |

```c
/* zombie_demo.c —— 制造一个持续 60s 的僵尸 */
#include <unistd.h>
#include <stdio.h>
int main(void) {
    pid_t pid = fork();
    if (pid == 0) { _exit(0); }        /* 子进程立刻退出     */
    /* 父进程不 wait，睡 60s —— 期间子进程一直是 Z */
    printf("child %d is zombie now, run: ps -o pid,stat,cmd -p %d\n", pid, pid);
    sleep(60);
    return 0;
}
```

> STAT 还会带修饰符：`Ss` = 会话首_leader、`Sl` = 多线程（带 NPTL 线程）、`R+` = 前台进程组、`<` = 高优先级（negative nice）、`N` = 低优先级。`ps axo pid,ppid,stat,wchan:20,cmd` 一次看全。

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
