## 8.5 信号（8.5.1–8.5.7）

### 8.5.1 信号术语

- **信号** — 软件层 **异步 ECF**；通知进程「某事件发生了」
- 每种信号有 **默认动作**：终止、停止、忽略、转储 core

常见：

| 信号 | 默认 | HFT 用途 |
|------|------|----------|
| `SIGINT` | 终止 | Ctrl+C，开发调试 |
| `SIGTERM` | 终止 | **优雅停机** |
| `SIGKILL` | 终止 | 强杀，**不可捕获** |
| `SIGCHLD` | 忽略 | 子进程退出 |
| `SIGPIPE` | 终止 | 写断开的 socket |
| `SIGUSR1/2` | 终止 | 自定义（reload 配置等） |

### 8.5.2 发送信号

```c
kill(pid, SIGTERM);
raise(SIGINT);
alarm(seconds);  // SIGALRM
```

- Shell **`Ctrl+C`** → 内核发 `SIGINT` 给前台进程组

### 8.5.3 接收信号

- 内核把信号标为 **待处理 (pending)**；到达时 **强制跳转** 到处理函数或默认动作
- `signal()` 过时；用 **`sigaction`** 指定 handler、标志

### 8.5.4 阻塞和解除阻塞

- **信号掩码** — `sigprocmask` 阻塞某类信号；**阻塞期间 pending，解除后交付**
- **`sigpending`** 查询待处理信号

### 8.5.5 编写信号处理程序

**规则（异步信号安全）：**

- 只能调 **async-signal-safe** 函数（见 `man 7 signal`）
- **禁止：** `printf`, `malloc`, `pthread_mutex_lock`（非安全列表很长）
- handler 里宜 **置 volatile sig_atomic_t 标志**，主循环处理

```c
volatile sig_atomic_t stop = 0;
void handler(int sig) { stop = 1; }

struct sigaction sa = { .sa_handler = handler, .sa_flags = SA_RESTART };
sigemptyset(&sa.sa_mask);
sigaction(SIGTERM, &sa, NULL);
```

### 8.5.6 同步流避免并发错误

- 信号可在 **任意指令间** 插入 — 与主程序 **真并发**，共享全局变量需 **`sig_atomic_t` 或阻塞信号** 保护
- 经典 bug：handler 与 main 同时改链表

### 8.5.7 显式等待信号

```c
sigsuspend(&mask);   // 原子等待
sigwait(&set, &sig); // 专用线程等信号 — HFT 更干净
```

**HFT 实践：**

- **热路径无 signal handler** — 用 **专用线程 `sigwait`** 处理 SIGTERM 触发 orderly shutdown
- **`SA_RESTART`** — 自动重启被中断的 syscall；或自己处理 EINTR
- 生产：**SIGTERM** 停收新单、flush 日志、join 线程，再 exit

### 自测题

<details>
<summary>1. 信号是同步的还是异步的？接收信号时会发生什么？</summary>

信号是**异步**的——可以在进程执行的任何时刻到达。内核在从内核态返回用户态前检查 pending 信号，如果有就修改栈帧让返回后跳转到信号处理函数。处理完通过 `sigreturn` 恢复原始执行流。

</details>

<details>
<summary>2. 信号处理函数中哪些函数是安全使用的？为什么不能在信号处理函数中调用 malloc？</summary>

只有**异步信号安全（async-signal-safe）**的函数可用（如 `write`, `_exit`, `read`）。`malloc` 不安全因为它操作全局堆数据结构，如果信号打断主线程正在 malloc 的代码，会导致堆损坏。**HFT 实践**：信号处理函数只设 flag（`volatile sig_atomic_t`），主循环检查 flag 处理。

</details>

<details>
<summary>3. HFT 系统中 SIGALRM 和 timer 有什么关系？</summary>

SIGALRM 由 `alarm()`/`setitimer()` 定时触发，可用于心跳检测或超时控制。但 HFT 通常不用信号做定时（信号处理有延迟且不可预测），而是用 `clock_gettime(CLOCK_MONOTONIC)` 主动轮询时间，或用 `timerfd_create` 把定时器变成 fd 用 epoll 统一管理。

</details>


---

← [本章导读](../README.md)
