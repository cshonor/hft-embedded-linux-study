# TLPI 第 33 章 — Threads: Further Details

**优先级**：🔴（信号×线程、fork 锁幽灵、NPTL 认知）——线程五部曲收官章
**前置**：[Ch29](../chapter-29-threads-intro/README.md)–[Ch32](../chapter-32-thread-cancellation/README.md) · [Ch22 sigwait](../chapter-22-signals-advanced/README.md) · [Ch28 fork](../chapter-28-process-creation-exec-detail/README.md)
**后置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/README.md) · [Ch35 调度](../chapter-35-process-priorities-scheduling/README.md)（daemon 见 [Ch37](../chapter-37-daemons/README.md)）

---

## 小节目录

- [33.1 线程栈](notes/33.1-thread-stacks.md) — 主线程 vs 子线程两套机制；16KB 内核栈
- [33.2 线程与信号（重难点）](notes/33.2-threads-and-signals.md) — complete_signal 投递算法；sigwait 架构
- [33.3 进程控制](notes/33.3-threads-and-process-control.md) — fork 锁幽灵；de_thread 窃 PID；exit 不对称
- [33.4 实现模型](notes/33.4-thread-implementation-models.md) — M:1 / 1:1 / M:N；协程是 M:1 转世
- [33.5 LinuxThreads vs NPTL](notes/33.5-linux-implementations-of-posix-threads.md) — 三大内核地基；pthread_t ≠ TID
- [33.6 高级同步（简介）](notes/33.6-advanced-features-of-the-pthreads-api.md) — rwlock/barrier/spin/PROCESS_SHARED
- [33.7 总结](notes/33.7-summary.md) — 速览 + 三个反直觉真相 + 铁律
- [33.8 练习](notes/33.8-exercises.md) — robust 锁 fork 存活 / barrier 分阶段 / mt_daemon 骨架

---

## 章节目标

一句话内核视角串起全章：**pthread 就是 clone 出的 task_struct**——由此推演出栈的两套机制（33.1）、信号的线程级掩码与轮转投递（33.2）、fork/exec/exit 的组语义（33.3）、1:1 模型（33.4-33.5）、落在 futex 上的全部同步原语（33.6）。

---

## 机制全景

```text
                     pthread_create
                          │ clone(CLONE_VM|SIGHAND|THREAD|SETTLS|CLEARTID...)
                          ▼
        ┌─────────── task_struct（每线程一个）───────────┐
        │ blocked 掩码（私有）   pending 队列（私有）      │ ← 33.2
        │ mm 共享（地址空间）     sighand 共享（处置表）    │
        │ 16KB 内核栈（物理）    fs base = TLS            │ ← 33.1 / 31.4
        │ robust 列表           futex 等待点             │ ← 33.8 / 30.x
        └───────────────────┬─────────────────────────┘
            fork: 只复制调用者（锁变幽灵）← 33.3     exec: de_thread 全杀+窃PID ← 33.3
```

---

## 易错清单

1. 多线程改掩码用 **`pthread_sigmask`**，勿靠 `sigprocmask`（MT 语义已废除）
2. 「掩码进程级共享」是**错觉**——处置共享，掩码每线程独立
3. 主线程 `return`/`exit` 杀光线程；要留 worker 用 `pthread_exit`
4. 多线程 fork → 子进程只准 async-signal-safe → 立刻 exec
5. `sigwait` 优于满地异步 handler；前置 = 调用线程已阻塞目标信号
6. `pthread_kill` 不能跨进程（`pthread_t` 本进程才有意义）
7. 子线程栈**不增长**（无 `VM_GROWSDOWN`）；主线程栈可以
8. 每线程 16KB 内核栈是**立即占用的物理内存，且不计入进程 RSS**
9. NPTL = 1:1；`pthread_t` ≠ 内核 TID，观测工具只认 TID
10. exec 杀线程**没有任何清理回调**（对比 cancel 的 cleanup 栈）——exec 前自己 quiesce
11. rwlock 默认**读者优先**——写延迟敏感场景会写饥饿
12. PROCESS_SHARED 忘设 → 跨进程**假同步**（futex key 各自哈希）
13. robust 锁是 fork 幽灵锁唯一能"检测存活"的方案（EOWNERDEAD + consistent）
14. `pthread_atfork` 的 child 回调里**重新初始化**锁，不是解锁无主锁
15. barrier 的 SERIAL_THREAD 每轮可能落在**不同线程**——不能假设固定收尾者

---

## 实验清单

1. ✅ `stack_probe`：主/子线程地址分区 + 默认栈属性（33.1）
2. ✅ `sig_model`：投递三定律（唯一解阻塞必收 / 定向只进一个 / sigwait 吞 handler）（33.2）
3. ✅ `proc_ctrl`：fork 锁幽灵 trylock 证据 + atfork 救场 + exec 杀心跳（33.3）
4. ✅ `sync_prim`：rwlock 并发读 / barrier 两轮集合 / spin 计数（33.6）
5. ✅ `fork_ghost`：normal/errchk/robust 三锁 fork 对照（33.8）
6. ✅ `mt_daemon`：sigwait 线程 + 优雅退出骨架（33.8）
7. （选）1000 线程 VmSize/RSS 对账（33.8 练习 5）

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 处置共享；掩码私有；pending 分进程级+线程级 |
| 2 | 阻塞信号 + `sigwait` 专用线程 = 信号处理标准架构 |
| 3 | complete_signal 从 curr_target 轮转找未阻塞者；全阻塞挂共享队列 |
| 4 | fork 只留调用线程；锁是幽灵；正解 = 立刻 exec / posix_spawn |
| 5 | exec：de_thread 全杀 + 非 leader 窃取 leader PID |
| 6 | exit/return = exit_group 全灭；主线程 pthread_exit 留活口 |
| 7 | 子线程栈 mmap 不增长；主线程栈 VM_GROWSDOWN 可扩 |
| 8 | 16KB 内核栈 × N = 立即物理内存（RSS 之外） |
| 9 | Linux = NPTL 1:1；pthread 就是 clone 的 task_struct |
| 10 | join 睡在 futex（CLONE_CHILD_CLEARTID 写 0 + WAKE） |
| 11 | rwlock 读者优先会写饥饿；短临界区 mutex 可能更快 |
| 12 | spinlock 快的前提：跨核 + 临界区 < 两次切换成本 |
| 13 | PROCESS_SHARED 设在 attr 上，忘了 = 假同步 |
| 14 | robust 锁 fork 后 EOWNERDEAD → consistent 可恢复 |
| 15 | pthread_t ≠ TID；perf/top -H/sched 认 TID |

---

## 参考

- Kerrisk · TLPI Ch33（Threads: Further Details）
- `man 3 pthread_sigmask` · `man 3 pthread_kill` · `man 3 sigwait` · `man 3 pthread_atfork` · `man 7 pthreads`
- 内核源码（v6.6）：`kernel/signal.c` · `fs/exec.c`（de_thread）· `kernel/fork.c`

---

## 前置知识依赖

- [Ch29](../chapter-29-threads-intro/README.md)（创建/属性/join）· [Ch30](../chapter-30-thread-synchronization/README.md)（mutex/condvar/futex）· [Ch31](../chapter-31-thread-safety-tsd/README.md)（TSD/TLS）· [Ch32](../chapter-32-thread-cancellation/README.md)（取消/cleanup）
- [Ch20-22 信号三部曲](../chapter-20-signals-fundamentals/README.md)
