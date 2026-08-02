# TLPI 第 22 章 — Signals: Advanced Features

> 对应目录：`chapter-22-signals-advanced/`  
> 书名原文：**Signals: Advanced Features**  
> ⚠️ **`pause` 有竞态；用 `sigsuspend`。** 多线程首选：阻塞信号 + `sigwaitinfo` 专用线程（避开 async-safe）。

**优先级**：🔴（可靠等待、实时信号、崩溃栈、服务进程信号模型）  
**前置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/notes.md)  
**后置**：[Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/notes.md) · [Ch29+ 线程](../chapter-29-threads-intro/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 章节目标

掌握 `pause`/`sigsuspend`、同步 `sigwait*`、`sigqueue` 实时信号、备用信号栈、`PR_SET_PDEATHSIG`；对比三种等待模型。

---

## 22.1 `pause()`

```c
int pause(void);   /* 永远 -1 / EINTR：等到某信号被捕获并跑完 handler */
```

**竞态：** `if (!flag) pause();` — 检查与休眠之间信号已到并置位 → 永久睡死。  
`pause` 不能原子地「放行某信号 + 休眠」。

---

## 22.2 `sigsuspend()`（核心）

```c
int sigsuspend(const sigset_t *mask);
```

原子步骤：

1. 将线程掩码**换成** `mask`  
2. 休眠等信号  
3. 捕获并跑完 handler 后，**恢复调用前掩码**  
4. 返回 `-1` / `EINTR`

安全范式：

```c
/* 1) 先 SIG_BLOCK 目标信号 */
/* 2) wait_mask = 当前掩码，但去掉要等的信号 */
while (!got_signal)
    sigsuspend(&wait_mask);
```

| | `sigprocmask` + `pause` | `sigsuspend` |
|--|-------------------------|--------------|
| 改掩码+睡 | 非原子 | **原子** |

Demo：[`code/sigsuspend_wait.c`](./code/sigsuspend_wait.c)

---

## 22.3 同步等待：`sigwait` 族

**不跑 handler**，从 pending 里同步取出信号。

```c
int sigwait(const sigset_t *set, int *sig);
int sigwaitinfo(const sigset_t *set, siginfo_t *info);
int sigtimedwait(const sigset_t *set, siginfo_t *info,
                 const struct timespec *timeout);
```

多线程推荐：

1. 关心的信号在所有线程上**先阻塞**  
2. 专用线程循环 `sigwaitinfo`  
3. 处理逻辑可 `malloc`/`printf`/加锁  

前提：目标信号必须已阻塞，否则可能先异步进 handler。

Demo：[`code/sigwaitinfo_loop.c`](./code/sigwaitinfo_loop.c)

---

## 22.4 实时信号与 `sigqueue`

```c
int sigqueue(pid_t pid, int sig, const union sigval value);
```

| vs `kill` | |
|-----------|--|
| 可带 `sigval`（int 或 ptr） | 跨进程 **勿传指针**（地址空间不同） |
| 可排队 | 队列满则失败 |
| 接收端 | `SA_SIGINFO`，读 `si_value` |

标准信号不排队；实时信号可排队，小号优先。

Demo：[`code/sigqueue_rt.c`](./code/sigqueue_rt.c)

---

## 22.5 备用信号栈

```c
int sigaltstack(const stack_t *ss, stack_t *old_ss);
/* sa_flags |= SA_ONSTACK */
```

默认栈耗尽时仍要跑 `SIGSEGV` handler → 预分配独立栈（大小常用 `SIGSTKSZ`）。

---

## 22.6 `prctl`（Linux）

```c
prctl(PR_SET_PDEATHSIG, sig);  /* 父死则本进程收 sig */
```

感知**直接父进程**死亡；常见于 worker 跟随父退出。

---

## 22.7 `EINTR` 再强调

不靠 `SA_RESTART` 包打天下；`connect`/`accept`/`sleep`/`epoll_wait` 等行为因调用而异 → **显式重试**。

---

## 22.8 三大等待对比

| 方式 | 模型 | 评价 |
|------|------|------|
| `pause` | 异步 + 睡 | 有竞态，正式代码少用 |
| `sigsuspend` | 异步 + 原子睡 | 修竞态；仍受 async-safe 约束 |
| `sigwaitinfo` | 同步取 pending | 无 async 限制；信号须先阻塞 |

---

## 22.9 易错清单

1. `sigsuspend` 只改**当前线程**掩码  
2. `sigwait` 前必须阻塞目标信号  
3. `sival_ptr` 跨进程无效  
4. 信号栈用够 `SIGSTKSZ`；`SA_ONSTACK` 按信号注册  
5. `PR_SET_PDEATHSIG` 只盯直接父  
6. `sigqueue` 队列满要处理失败  

---

## 练习

1. 复现 `pause` 竞态，用 `sigsuspend` 修  
2. `sigwaitinfo` 循环取信号  
3. `sigqueue` 传 int  
4. （选）`sigaltstack` + `SA_ONSTACK`  
5. （选）`PR_SET_PDEATHSIG`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `pause` 竞态；用 `sigsuspend` |
| 2 | `sigsuspend` = 换掩码 + 睡（原子） |
| 3 | 服务端：阻塞 + `sigwaitinfo` 线程 |
| 4 | `sigqueue` + `SA_SIGINFO` 传数据；跨进程用 int |
| 5 | 栈溢出收尸用 `sigaltstack` + `SA_ONSTACK` |
| 6 | 勿迷信 `SA_RESTART`；处理 `EINTR` |

---

## 参考

- Kerrisk · TLPI Ch22  
- `man 2 sigsuspend` · `man 3 sigwaitinfo` · `man 3 sigqueue` · `man 2 sigaltstack` · `man 2 prctl`
