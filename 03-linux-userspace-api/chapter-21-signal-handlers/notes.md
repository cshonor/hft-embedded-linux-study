# TLPI 第 21 章 — Signals: Signal Handlers

> 对应目录：`chapter-21-signal-handlers/`  
> 书名原文：**Signals: Signal Handlers**  
> ⚠️ **首选 `sigaction()`；** handler 内只做异步信号安全操作。推荐范式：设 `volatile sig_atomic_t`，主循环干活。

**优先级**：🔴（崩溃/死锁/EINTR/僵尸回收）  
**前置**：[Ch20 信号基础](../chapter-20-signals-fundamentals/notes.md)  
**后置**：[Ch22 pause / sigsuspend](../chapter-22-signals-advanced/notes.md) · [Ch24 进程创建 / wait](../chapter-24-process-creation/notes.md)

---

## 章节目标

用 `sigaction` 注册处理器；理解 `sa_mask` 与 `SA_*`；严守 async-signal-safe；处理 `EINTR` / `SA_RESTART`；安全处理 `SIGCHLD`。

---

## 21.1 `sigaction()`

```c
#include <signal.h>
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
```

不能用于 `SIGKILL` / `SIGSTOP`。`act`/`oldact` 可为 NULL。

```c
struct sigaction {
    void (*sa_handler)(int);                          /* 或 */
    void (*sa_sigaction)(int, siginfo_t *, void *); /* 需 SA_SIGINFO */
    sigset_t sa_mask;
    int sa_flags;
};
```

| 形式 | 用途 |
|------|------|
| `sa_handler` | 仅信号编号；可赋 `SIG_DFL` / `SIG_IGN` |
| `sa_sigaction` + `SA_SIGINFO` | `siginfo_t`（pid/uid/来源/`si_value` 等）；`sigqueue` 传值必备 |

Demo：[`code/flag_handler.c`](./code/flag_handler.c) · [`code/siginfo_demo.c`](./code/siginfo_demo.c)

---

## 21.2 `sa_mask`（临时叠加）

handler 运行期间：

1. 默认**自动阻塞**正在处理的那个信号（防递归）  
2. **额外**阻塞 `sa_mask`  
3. handler 返回后恢复原线程掩码  

| | `sigprocmask` | `sa_mask` |
|--|---------------|-----------|
| 时效 | 长期（直到再改） | **仅 handler 期间** |
| 作用 | 改线程掩码 | 临时追加阻塞 |

---

## 21.3 常用 `sa_flags`

| 标志 | 含义 |
|------|------|
| `SA_SIGINFO` | 用 `sa_sigaction` |
| `SA_RESTART` | 多数慢调用被中断后自动重启（否则常 `EINTR`） |
| `SA_NODEFER` | 不自动阻塞本信号（易重入，慎用） |
| `SA_RESETHAND` | 一次后复位 `SIG_DFL`（老 `signal` 味） |
| `SA_NOCLDSTOP` | `SIGCHLD`：子停/续不通知 |
| `SA_NOCLDWAIT` | `SIGCHLD`：子退出不留僵尸 |

---

## 21.4 `EINTR` 与 `SA_RESTART`

阻塞在慢系统调用时递送信号 → 跑完 handler 后：

- 默认：调用失败，`errno == EINTR`  
- `SA_RESTART`：许多调用自动重启  

⚠️ **并非所有**调用都可重启（`epoll_wait`、部分 `accept`/`sleep` 等因平台而异）。  
业务应**统一处理 `EINTR`**（重试或显式退出）。

Demo：[`code/eintr_read.c`](./code/eintr_read.c)

---

## 21.5 Async-Signal-Safe（最重要）

handler 可随时打断用户态代码 → **禁止**非异步信号安全函数。

| ✅ 常见允许 | ❌ 严禁 |
|-------------|--------|
| `write`、`_exit`、`sigaction`、`sigprocmask`、`sigpending`、…（见 POSIX 列表） | `printf`、`malloc`/`free`、stdio、多数锁、C++/库随意调用 |

推荐范式：

```c
volatile sig_atomic_t got_sigint = 0;
void handler(int sig) { (void)sig; got_sigint = 1; }
```

`sig_atomic_t`：读写在实现上视为原子，避免撕裂。

---

## 21.6 `SIGCHLD`

子进程退出 → 父收 `SIGCHLD`。

| 做法 | |
|------|--|
| handler 里 | 循环 `waitpid(-1, …, WNOHANG)` 收尸（标准信号不排队，一次可能对应多个子） |
| `SA_NOCLDWAIT` | 内核自动收，不留僵尸 |

Demo：[`code/sigchld_reap.c`](./code/sigchld_reap.c)

---

## 21.7 `signal()` vs `sigaction()`

| | `signal()` | `sigaction()` |
|--|------------|---------------|
| `sa_mask` / flags | ❌ / 不一致 | ✅ |
| `SA_SIGINFO` | ❌ | ✅ |
| `SA_RESTART` | 实现不一 | 可显式设 |
| 生产 | 弃用 | **首选** |

---

## 21.8 易错清单

1. `sa_mask` 叠加，不替换；退出即撤  
2. `SA_NODEFER` → 易递归  
3. `EINTR` 合法，别当致命  
4. 标志用 `volatile sig_atomic_t`  
5. handler 禁 `printf`/`malloc`  
6. 实时信号 + `sigqueue` 需 `SA_SIGINFO`  
7. 多线程：信号交给**未阻塞该信号**的某线程  

---

## 练习

1. `sigaction` + `sa_mask`  
2. 开关 `SA_RESTART` 看 `read`/`EINTR`  
3. `SA_SIGINFO` 打印发送者 pid  
4. `sig_atomic_t` 主循环范式  
5. `SIGCHLD` 循环 `waitpid`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 用 `sigaction`，不用 `signal` |
| 2 | handler 期间：自动阻本信号 + `sa_mask` |
| 3 | 只设 `volatile sig_atomic_t`；禁非 async-safe |
| 4 | 处理 `EINTR`；`SA_RESTART` 非万能 |
| 5 | `SIGCHLD`：循环 `WNOHANG` 或 `SA_NOCLDWAIT` |
| 6 | `SA_SIGINFO` 才拿得到 `siginfo_t` / 队列数据 |

---

## 参考

- Kerrisk · TLPI Ch21  
- `man 2 sigaction` · `man 7 signal-safety` · `man 2 waitpid`
