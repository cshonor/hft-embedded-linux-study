# TLPI 第 22 章 — Signals: Advanced Features

**优先级**：🔴（可靠等待、实时信号、崩溃栈、服务进程信号模型）
**前置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md)
**后置**：[Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/README.md) · [Ch29+ 线程](../chapter-29-threads-intro/README.md) · Ch63 多路 I/O

---

## 小节目录

- [22.1 Core Dump 文件](notes/22.1-core-dump-files.md)
- [22.2 递送、处置与挂起的特殊情况](notes/22.2-special-cases-for-delivery-disposition-a.md)
- [22.3 可中断与不可中断的进程睡眠状态](notes/22.3-interruptible-and-uninterruptible-proces.md)
- [22.4 硬件产生的信号](notes/22.4-hardware-generated-signals.md)
- [22.5 同步与异步的信号生成](notes/22.5-synchronous-and-asynchronous-signal-gene.md)
- [22.6 递送的时序与顺序](notes/22.6-timing-and-order-of-signal-delivery.md)
- [22.7 `signal()` 的实现与可移植性](notes/22.7-implementation-and-portability-of-signal.md)
- [22.8 实时信号](notes/22.8-realtime-signals.md)
- [22.9 使用掩码等待信号：`sigsuspend()`](notes/22.9-waiting-for-a-signal-using-a-mask-sigsus.md)
- [22.10 同步等待信号（`sigwaitinfo`）](notes/22.10-synchronously-waiting-for-a-signal.md)
- [22.11 通过文件描述符获取信号（`signalfd`）](notes/22.11-fetching-signals-via-a-file-descriptor.md)
- [22.12 用信号做进程间通信](notes/22.12-interprocess-communication-with-signals.md)
- [22.13 早期信号 API（System V 和 BSD）](notes/22.13-earlier-signal-apis-system-v-and-bsd.md)
- [22.14 本章小结](notes/22.14-summary.md)
- [22.15 练习题](notes/22.15-exercises.md)

---

## 章节目标

本章回答 Ch20/Ch21 讲完基础后必然冒出的四个问题：

1. **时序**：信号到底什么时候递送？多个 pending 谁先谁后？为什么 `kill -9` 有时杀不掉？（22.2 / 22.3 / 22.6）
2. **接收方式**：除了装 handler，还有 `sigsuspend` / `sigwaitinfo` / `signalfd` 三条路——各自适用什么场景、为什么 `signalfd` 抓不到 `SIGSEGV`。（22.9 / 22.10 / 22.11）
3. **硬件异常**：段错误、除零这类「CPU 自己产生的信号」与 `kill()` 在内核里走同一条路吗？（22.4 / 22.5）
4. **历史包袱**：`signal()` / `sigset()` / `sigpause()` 的来路，以及为什么生产代码一律用 `sigaction`。（22.7 / 22.13）

---

## 三种接收方式速览

| 方式 | 跑 handler？ | 能进 epoll？ | 能抓 `SIGSEGV`？ | 典型用途 |
|------|--------------|--------------|------------------|----------|
| `sigaction` handler | ✅ 是 | ❌ | ✅ **唯一能抓** | 崩溃兜底、栈溢出收尸 |
| `sigsuspend(&mask)` | ✅ 是 | ❌ | ⚠️ 理论可以但不该 | 单线程原子等待 |
| `sigwaitinfo` / `sigtimedwait` | ❌ 否 | ❌ | ❌ | 多线程专用信号线程（推荐） |
| `signalfd` + `read` | ❌ 否 | ✅ **是** | ❌ | 主事件循环统一纳管 |

**关键约束**：`signalfd` / `sigwaitinfo` 之前**必须先 `sigprocmask(SIG_BLOCK, ...)`**，否则信号走默认动作或 handler，fd 上永远没数据。

---

## 易错清单

1. `sigsuspend` 的参数是**睡眠期间的完整掩码**，不是「要等哪些信号」——传「只含目标信号」的集合是典型错误
2. `pause()` 有致命竞态（解除阻塞 → 信号到达 → 永远睡着）；一律用 `sigsuspend`
3. `sigwaitinfo` / `signalfd` 之前**必须先阻塞**目标信号，否则信号被 handler 或默认动作消费掉
4. `signalfd` 的 `read` 缓冲必须 ≥ `sizeof(struct signalfd_siginfo)`，否则返回 **`-EINVAL`**（不是 `ENOBUFS`）
5. `signalfd` / `sigwaitinfo` **抓不到** `SIGSEGV`/`SIGBUS`/`SIGFPE`/`SIGILL`——同步故障信号走 `force_sig_info_to_task():1336` 线程定向直投，只能 `sigaction` + `SA_ONSTACK` 兜底
6. 标准信号**不排队**（`legacy_queue():1079`）：`SIGCHLD` 必须 `while (waitpid(-1, NULL, WNOHANG) > 0);`
7. 多个 pending 信号的递送顺序是**信号编号小的优先**（`next_signal():209`），不是 FIFO
8. `kill -9` 杀不掉 `TASK_UNINTERRUPTIBLE`（`D` 状态）进程——信号挂上了但进程叫不醒
9. `SA_ONSTACK` 是**按信号**注册的，不是全局开关；每个崩溃信号都要单独设，且要先 `sigaltstack`
10. 崩溃 handler 不能正常返回（CPU 会重试肇事指令 → 无限循环），写完现场后 `_exit` 或 re-raise
11. `SIGKILL`/`SIGTERM` **不在** `SIG_KERNEL_COREDUMP_MASK` 里——要留 core 用 `kill -QUIT`
12. `signal()` 在 Linux 内核层（`sys_signal:4622`）和 glibc 层语义**相反**；跨 UNIX 更不可移植。生产一律 `sigaction` 并显式写 `sa_flags`
13. `sigpause` **同名不同义**：BSD 版参数是掩码、SysV 版参数是信号编号，参数都是 `int`，编译器无法区分
14. 从 BSD 老代码迁移 `sigaction` 时最易漏 `SA_RESTART`——两派默认值相反，漏了会偶发 `EINTR`
15. 内核对未识别的 `sa_flags` **静默清除不报错**（`do_sigaction():4153-4156`），唯一可靠验证是装完读回比对

---

## 练习

1. 复现 `pause` 竞态，用 `sigsuspend` 修复
2. `sigwaitinfo` 循环取信号（注意 `EAGAIN` 超时 vs `EINTR` 打断）
3. `sigqueue` 传 `int`，并测试队列溢出（`EAGAIN`）
4. `signalfd` + `epoll` 统一事件循环
5. （选）`sigaltstack` + `SA_ONSTACK` 处理栈溢出
6. （选）诊断 `D` 状态进程：`ps -eo stat,wchan` + `/proc/PID/status` 的 `ShdPnd`
7. （选）遗留 API 迁移审查：`grep -nE '\b(signal|sigset|sighold|sigpause|sigblock|sigvec)\s*\('`

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `pause` 有竞态；等待信号用 `sigsuspend`（换掩码 + 睡，原子） |
| 2 | `sigwaitinfo` **取走**信号（不跑 handler）；`sigsuspend` **递送**信号（跑 handler） |
| 3 | 同步等待 / `signalfd` 前**必须阻塞**目标信号 |
| 4 | 多线程标准模型：main 阻塞全部 → 起一个 `sigwaitinfo` 线程 |
| 5 | `SIGSEGV` 等同步故障信号只能 handler 抓，`signalfd` 抓不到 |
| 6 | 标准信号不排队；`SIGCHLD` 要循环 `waitpid` |
| 7 | 递送顺序：号小者优先（同步信号除外，它们最优先） |
| 8 | `D` 状态（`UNINTERRUPTIBLE`）连 `SIGKILL` 也叫不醒 |
| 9 | `sigqueue` + 实时信号可排队可带数据；跨进程**不能**传指针 |
| 10 | 生产代码不用 `signal()`；`sa_flags` 显式写，装完读回验证 |

---

## 参考

- Kerrisk · TLPI Ch22
- `man 2 sigsuspend` · `man 3 sigwaitinfo` · `man 3 sigqueue` · `man 2 signalfd` · `man 2 sigaltstack` · `man 5 core` · `man 7 signal`

---

## 代码示例

```c
/* ch22_demo.c —— Ch22 核心：三种接收方式对照
 *
 *   A. sigsuspend  原子等待（单线程）
 *   B. sigwaitinfo 同步取走（不跑 handler，多线程推荐）
 *   C. signalfd    fd 化，可进 epoll
 *
 * 编译: gcc -Wall -Wextra -O2 -o ch22_demo ch22_demo.c
 * 运行: ./ch22_demo
 *       kill -USR1 $(pidof ch22_demo)
 *       kill -TERM $(pidof ch22_demo)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/signalfd.h>

static volatile sig_atomic_t got = 0;

/* handler 只置标志 —— 唯一允许的通信方式 */
static void on_sig(int sig) { got = sig; }

static int install(int sig, int flags)
{
    struct sigaction sa, verify;

    memset(&sa, 0, sizeof(sa));          /* ① 清零（含 sa_mask）*/
    sa.sa_handler = on_sig;
    sigemptyset(&sa.sa_mask);            /* ② 显式初始化，双保险 */
    sa.sa_flags   = flags;               /* ③ 显式，不依赖默认值 */
    if (sigaction(sig, &sa, NULL) == -1)
        return -1;

    /* ④ 读回校验 —— 内核会静默清除未知 flag（do_sigaction():4153-4156）*/
    sigaction(sig, NULL, &verify);
    if (verify.sa_handler != on_sig || (verify.sa_flags & flags) != (unsigned)flags)
        return -1;
    return 0;
}

/* ── A. sigsuspend：换掩码 + 睡，原子 ───────────────────── */
static void demo_sigsuspend(void)
{
    sigset_t block, prev, wait;

    install(SIGUSR1, SA_RESTART);

    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigprocmask(SIG_BLOCK, &block, &prev);       /* 先阻塞 */

    wait = prev;                                  /* 取阻塞前的掩码 */
    sigdelset(&wait, SIGUSR1);                    /* 放开目标信号 */

    printf("[A] sigsuspend: 等待 SIGUSR1 ...\n");
    fflush(stdout);
    sigsuspend(&wait);                            /* 原子：换掩码 + 睡 */
    printf("[A] 收到信号 %d\n", got);
}

/* ── B. sigwaitinfo：直接取走，不跑 handler ──────────────── */
static void demo_sigwaitinfo(void)
{
    sigset_t set;
    siginfo_t info;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);           /* 必须先阻塞 */

    kill(getpid(), SIGUSR2);                      /* 自己发一个 */

    printf("[B] sigwaitinfo: 阻塞取 SIGUSR2 ...\n");
    fflush(stdout);
    int sig = sigwaitinfo(&set, &info);
    if (sig > 0)
        printf("[B] 取到信号 %d (si_pid=%d)，handler 未被调用 (got=%d)\n",
               sig, info.si_pid, got);
}

/* ── C. signalfd：fd 化，可进 epoll ──────────────────────── */
static void demo_signalfd(void)
{
    sigset_t mask;
    struct signalfd_siginfo si;

    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);

    sigprocmask(SIG_BLOCK, &mask, NULL);          /* ① 必须先阻塞 */
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK);  /* ② 建 fd */
    if (sfd == -1) { perror("signalfd"); return; }

    kill(getpid(), SIGTERM);

    printf("[C] signalfd: read 一个 signalfd_siginfo ...\n");
    fflush(stdout);

    /* ③ 缓冲区必须 ≥ sizeof(signalfd_siginfo)，否则 -EINVAL */
    ssize_t n = read(sfd, &si, sizeof(si));
    if (n == (ssize_t)sizeof(si))
        printf("[C] 读到信号 %u (sender pid=%u)\n", si.ssi_signo, si.ssi_pid);
    else if (n == -1)
        perror("[C] read");

    close(sfd);
}

int main(void)
{
    printf("pid=%d\n\n", (int)getpid());
    demo_sigsuspend();
    demo_sigwaitinfo();
    demo_signalfd();

    printf("\n对比要点:\n"
           "  sigsuspend  → 跑 handler（got 被置位）\n"
           "  sigwaitinfo → 不跑 handler，信号被取走\n"
           "  signalfd    → 不跑 handler，可被 epoll 统一纳管\n"
           "  三者都必须【先阻塞】目标信号才不会丢\n");
    return 0;
}
```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
