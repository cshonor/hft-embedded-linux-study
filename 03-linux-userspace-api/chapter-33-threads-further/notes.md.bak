# TLPI 第 33 章 — Threads: Further Details

> 对应目录：`chapter-33-threads-further/`  
> （勿用 `chapter-33-threads-further-details` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 不一致）  
> 书名原文：**Threads: Further Details**  
> ⚠️ **多线程信号：处置进程共享；掩码每线程。** 用 `pthread_sigmask` + 专用 `sigwait` 线程。多线程 `fork` 只留调用线程 → 立刻 `exec`。

**优先级**：🔴（信号×线程、fork、NPTL 认知）  
**前置**：[Ch29](../chapter-29-threads-intro/notes.md)–[Ch32](../chapter-32-thread-cancellation/notes.md) · [Ch22 sigwait](../chapter-22-signals-advanced/notes.md) · [Ch28 fork](../chapter-28-process-creation-exec-detail/notes.md)  
**后置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/notes.md)（线程模块收束；daemon 见 [Ch37](../chapter-37-daemons/notes.md)）

---

## 章节目标

线程栈属性；多线程信号模型；fork/exec/exit；`pthread_atfork`；M:1/1:1/M:N 与 NPTL；读写锁/屏障/自旋锁速览。

---

## 33.1 线程栈

每线程私有栈；默认大小受 `RLIMIT_STACK` 等影响。

```c
pthread_attr_setstacksize / getstacksize
pthread_attr_setstack          /* 自备栈，少用 */
sysconf(_SC_THREAD_STACK_MIN)
```

栈溢出 → `SIGSEGV`；海量线程吃地址空间 → 可调小栈；深递归 → 加大。

---

## 33.2 线程与信号（重难点）

| 进程级（共享） | 线程级（私有） |
|----------------|----------------|
| `sigaction` 处置 / 默认动作 | **信号掩码**（`pthread_sigmask`） |
| 进程 pending（标准信号不排队） | 线程 pending；递送目标线程 |

```c
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
int pthread_kill(pthread_t thread, int sig);   /* 仅同进程内 */
int sigwait(const sigset_t *set, int *sig);    /* 同步取信号 */
```

| 投递 | |
|------|--|
| 异步信号（SIGINT 等） | 任选**未阻塞该信号**的线程 |
| `pthread_kill` | 指定线程 |
| handler | 跑在收到信号的那条线程上 |

**推荐：** 创建业务线程前主线程阻塞关心信号 → 专用线程 `sigwait` → 业务线程不装异步 handler。

Demo：[`code/thread_sigwait.c`](./code/thread_sigwait.c)

---

## 33.3 进程控制

| 调用 | 多线程行为 |
|------|------------|
| **fork** | 子进程只留调用线程；锁状态危险 → **立刻 exec**；`pthread_atfork` 仅缓解 |
| **exec** | 只留调用线程；handler→DFL |
| **exit** / main return | **整进程**全线程死 |
| 只退主线程 | 主线程 `pthread_exit` |

```c
pthread_atfork(prepare, parent, child);
```

---

## 33.4 实现模型

| 模型 | 要点 |
|------|------|
| M:1 | 用户级；快；难多核；阻塞拖死全体 |
| **1:1** | Linux NPTL；每 pthread ≈ 一内核任务；可多核 |
| M:N | 复杂；Linux 未走 |

---

## 33.5 LinuxThreads vs NPTL

| | LinuxThreads（废） | **NPTL**（现行） |
|--|-------------------|------------------|
| PID | 每线程一 PID（非 POSIX） | 同进程 TGID 相同；独立 TID |
| 信号 | 怪异 | POSIX 语义 |
| 同步 | — | 常基于 futex |

勿混：`pthread_t` ≠ 内核 TID（`gettid`）。

---

## 33.6 高级同步（简介）

| 原语 | 用途 |
|------|------|
| `pthread_rwlock_t` | 读共享、写独占 |
| `pthread_barrier_t` | 集合点；一人得 `PTHREAD_BARRIER_SERIAL_THREAD` |
| `pthread_spinlock_t` | 短临界区忙等 |
| `PTHREAD_PROCESS_SHARED` | 放共享内存可跨进程 |

Demo：[`code/pthread_barrier_demo.c`](./code/pthread_barrier_demo.c) · [`code/pthread_rwlock_demo.c`](./code/pthread_rwlock_demo.c)

---

## 易错清单

1. 多线程改掩码用 **`pthread_sigmask`**，勿靠 `sigprocmask`  
2. 主线程 `return`/`exit` 杀光线程  
3. 多线程 fork → 立刻 exec  
4. `sigwait` 优于满地异步 handler  
5. `pthread_kill` 不能跨进程  
6. NPTL = 1:1；TID ≠ `pthread_t`  

---

## 实验清单

1. （选）`attr` 设栈大小  
2. `sigwait` 信号线程  
3. （选）`pthread_kill`  
4. barrier / rwlock  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | handler 共享；掩码每线程 |
| 2 | 阻塞信号 + `sigwait` 专用线程 |
| 3 | fork 只留一线程；立刻 exec |
| 4 | exit 杀进程；主线程可 `pthread_exit` |
| 5 | Linux = NPTL 1:1 |
| 6 | rwlock / barrier / spin 按场景用 |

---

## 参考

- Kerrisk · TLPI Ch33  
- `man 3 pthread_sigmask` · `man 3 pthread_kill` · `man 3 sigwait` · `man 3 pthread_atfork` · `man 7 pthreads`
