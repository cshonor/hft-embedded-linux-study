# TLPI 第 21 章 — Signals: Signal Handlers

**优先级**：🔴（崩溃/死锁/EINTR/僵尸回收）  
**前置**：[Ch20 信号基础](../chapter-20-signals-fundamentals/README.md)  
**后置**：[Ch22 pause / sigsuspend](../chapter-22-signals-advanced/README.md) · [Ch24 进程创建 / wait](../chapter-24-process-creation/README.md)

---

## 小节目录

- [21.1 `sigaction()`](notes/21.1-designing-signal-handlers.md)
- [21.2 `sa_mask`（临时叠加）](notes/21.2-other-methods-of-terminating-a-signal-ha.md)
- [21.3 常用 `sa_flags`](notes/21.3-handling-a-signal-on-an-alternate-stack-.md)
- [21.4 `EINTR` 与 `SA_RESTART`](notes/21.4-the-sa-siginfo-flag.md)
- 21.5 Async-Signal-Safe（最重要）
- [21.6 `SIGCHLD`](notes/21.6-summary.md)
- [21.7 `signal()` vs `sigaction()`](notes/21.1-designing-signal-handlers.md)

---

## 章节目标


用 `sigaction` 注册处理器；理解 `sa_mask` 与 `SA_*`；严守 async-signal-safe；处理 `EINTR` / `SA_RESTART`；安全处理 `SIGCHLD`。

---


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


---

## 练习


1. `sigaction` + `sa_mask`  
2. 开关 `SA_RESTART` 看 `read`/`EINTR`  
3. `SA_SIGINFO` 打印发送者 pid  
4. `sig_atomic_t` 主循环范式  
5. `SIGCHLD` 循环 `waitpid`  

---


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


---

## 参考


- Kerrisk · TLPI Ch21  
- `man 2 sigaction` · `man 7 signal-safety` · `man 2 waitpid`


---

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/* Ch21 信号处理器 — sigaction（替代可移植性差的 signal）。
 * 演示 sigaction 注册 + 信号信息获取。
 * 编译: gcc -o ch21_demo ch21_demo.c */

static volatile sig_atomic_t count = 0;

void handler(int sig, siginfo_t *info, void *ctx) {
    count++;
    /* 注意: printf 不是异步信号安全函数，这里仅做演示 */
    const char *msg = "caught SIGINT\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    /* sigaction 比 signal 更可移植、更强大 */
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    printf("Press Ctrl+C to test (will catch 3 times then exit)\n");
    printf("Or run: kill -INT %d\n", (int)getpid());

    while (count < 3) {
        pause();  /* 等待信号 */
    }
    printf("Caught %d signals, exiting.\n", count);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
