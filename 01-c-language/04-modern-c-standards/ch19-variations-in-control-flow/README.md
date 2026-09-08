# Ch19 · Variations in control flow（控制流的变化）

> **Level 3 · 深入** · 策略：**🟡 略读**（信号处理器限制要看）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

顺序执行与副作用、短跳转（`goto`）、函数与尾调用、**长跳转 `setjmp`/`longjmp`**、
**信号处理器 `signal`/`sigaction` 的 async-signal-safe 限制**。

## 一、`setjmp` / `longjmp`

### 基本用法

`setjmp` 保存当前执行上下文（栈指针、寄存器等），`longjmp` 恢复它——实现非局部跳转。

```c
#include <setjmp.h>

jmp_buf env;

int risky_operation(void) {
    if (something_bad)
        longjmp(env, 1);    // 跳回 setjmp 处，返回值为 1
    return 0;
}

int main(void) {
    if (setjmp(env) == 0) {
        /* 正常路径 */
        risky_operation();
        printf("success\n");
    } else {
        /* longjmp 跳回这里 */
        printf("error recovered\n");
    }
    return 0;
}
```

| 要点 | 说明 |
|------|------|
| `setjmp` 返回值 | 第一次调用返回 0；`longjmp` 跳回时返回 `longjmp` 的第二个参数（非 0） |
| 保存的上下文 | 栈指针、指令指针、寄存器（不保存信号掩码，除非用 `sigsetjmp`） |
| 限制 | 不能跳回已退出的函数（栈帧已销毁）→ UB |
| 变量值 | `volatile` 局部变量保持 `longjmp` 时的值；非 volatile 可能是垃圾值 |

### `setjmp`/`longjmp` 的陷阱

```c
/* ❌ 非 volatile 变量在 longjmp 后值不确定 */
int count = 0;
if (setjmp(env) == 0) {
    count = 42;
    risky_operation();    // longjmp(env, 1)
}
printf("%d\n", count);    // count 可能是 42，也可能被回滚 → UB

/* ✅ 加 volatile 保证值不被回滚 */
volatile int count = 0;
if (setjmp(env) == 0) {
    count = 42;
    risky_operation();
}
printf("%d\n", count);    // ✅ 一定是 42
```

> **HFT 场景**：`longjmp` 跳出错误恢复路径——解析协议时遇到坏数据包，跳回错误处理点。
> 但现代 C 倾向用返回值/错误码替代 `setjmp`/`longjmp`，更安全可控。

## 二、信号处理（重点）

### 信号基础

```c
#include <signal.h>

/* 注册信号处理器 */
void handler(int sig) {
    /* ⚠ 只能调用 async-signal-safe 函数！ */
    write(STDERR_FILENO, "caught SIGINT\n", 14);
}

int main(void) {
    signal(SIGINT, handler);      /* 简单注册（可移植性差） */
    signal(SIGTERM, handler);

    /* sigaction 更可移植、功能更强 */
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    while (1) pause();   /* 等待信号 */
    return 0;
}
```

| `signal()` vs `sigaction()` | 说明 |
|-----------------------------|------|
| `signal()` | 简单但行为不一致（BSD vs SysV 语义不同） |
| `sigaction()` | 可移植、可控制信号掩码和标志（`SA_RESTART` 等） |
| HFT 建议 | 用 `sigaction()`，不用 `signal()` |

### async-signal-safe 限制（核心）

**信号处理器运行在特殊的上下文中**——它可能打断了正在执行的任何代码。
因此只能调用 **async-signal-safe** 函数。

```c
/* ❌ 信号处理器中禁止的函数 */
void bad_handler(int sig) {
    printf("...");        // ❌ printf 不是 async-signal-safe（内部有锁/缓冲）
    malloc(100);          // ❌ malloc 不是 async-signal-safe（全局锁）
    free(ptr);            // ❌ free 同上
    syslog(LOG_INFO, ""); // ❌ syslog 不是 async-signal-safe
    fprintf(stderr, "");  // ❌ 同 printf
}

/* ✅ 信号处理器中安全的函数 */
void good_handler(int sig) {
    volatile sig_atomic_t flag = 1;   // ✅ 只设 flag
    write(STDERR_FILENO, "sig\n", 4); // ✅ write 是 async-signal-safe
    _exit(1);                          // ✅ _exit 是 async-signal-safe
}
```

| 常见 async-signal-safe 函数 | 常见不安全函数 |
|---------------------------|---------------|
| `write` | `printf` / `fprintf` |
| `read` | `malloc` / `free` |
| `_exit` | `exit`（会调 atexit + 刷新 stdio） |
| `signal` / `sigaction` | `syslog` |
| `open` / `close` | `fopen` / `fclose` |

> **完整列表**：`man 7 signal-safety` 或 POSIX 标准。

### HFT 信号处理模式

```c
/* HFT 进程的标准信号处理模式 */
static volatile sig_atomic_t should_stop = 0;

static void signal_handler(int sig) {
    (void)sig;
    should_stop = 1;   // 只设 flag，不做任何复杂操作
}

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);    /* Ctrl+C */
    sigaction(SIGTERM, &sa, NULL);   /* kill */

    /* 主循环检查 flag */
    while (!should_stop) {
        process_packets();
    }

    /* 优雅退出：在信号上下文之外做清理 */
    cleanup_resources();
    return 0;
}
```

| 规则 | 说明 |
|------|------|
| 只设 `volatile sig_atomic_t` flag | 信号处理器里唯一的操作 |
| 主循环检查 flag | 在正常上下文中做清理 |
| 不用 `printf`/`malloc` | 不是 async-signal-safe |
| `sig_atomic_t` | 保证读写不被中断的整数类型 |

### HFT 行情进程的自救/重启逻辑

```c
/* HFT 进程遇到致命错误时的处理 */
static volatile sig_atomic_t restart_needed = 0;

void segv_handler(int sig, siginfo_t *info, void *context) {
    (void)sig; (void)info; (void)context;
    /* 写崩溃日志（用 async-signal-safe 的 write） */
    const char msg[] = "SEGFAULT: initiating restart\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    restart_needed = 1;
    /* 跳回主循环（用 siglongjmp 或设 flag） */
}

/* 主进程监控子进程，崩溃后自动重启 */
int main(void) {
    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程：运行 HFT 引擎 */
            setup_signal_handlers();
            run_engine();
            _exit(0);
        } else {
            /* 父进程：等待子进程 */
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                break;   /* 正常退出 */
            /* 崩溃则重启 */
            log_crash(status);
        }
    }
}
```

## 三、尾调用

```c
/* 尾递归：递归调用是最后一步 */
int factorial_tail(int n, int acc) {
    if (n <= 1) return acc;
    return factorial_tail(n - 1, n * acc);   // 尾调用
}

/* 编译器可能优化为循环（不压栈）：
   while (n > 1) { acc *= n; n--; }
   return acc;
*/
```

| 要点 | 说明 |
|------|------|
| 尾调用优化 (TCO) | 编译器把尾调用转为循环，不增加栈深度 |
| 不保证 | C 标准不要求编译器做 TCO（gcc -O2 会做） |
| HFT 意义 | 深递归可改为尾递归避免栈溢出，但热路径建议直接用迭代 |

## HFT / DPDK 关联

| 概念 | HFT 应用 |
|------|----------|
| `setjmp`/`longjmp` | 错误恢复路径（协议解析遇到坏包跳出） |
| 信号处理器 | 优雅退出（Ctrl+C → 设 flag → 主循环清理） |
| async-signal-safe | 信号处理器里只能用 `write`/`_exit`/设 flag |
| `volatile sig_atomic_t` | 信号 flag 的正确类型 |
| 进程监控+重启 | 父进程 fork 子进程，崩溃后自动重启 |

## 自测题

<details><summary>1. 信号处理器里为什么不能调用 <code>printf</code>？</summary>

`printf` 不是 async-signal-safe——它内部使用 stdio 缓冲区（有锁），如果信号打断了正在执行
`printf` 的代码，信号处理器里再次调用 `printf` 会死锁或损坏缓冲区。同理 `malloc`/`free` 有全局锁，
`exit` 会调 atexit handler 和刷新 stdio。信号处理器只能调用 async-signal-safe 函数
（`write`/`read`/`_exit` 等），或只设置 `volatile sig_atomic_t` flag。
</details>

<details><summary>2. <code>setjmp</code> 后 <code>longjmp</code> 回来时，局部变量的值可靠吗？</summary>

不一定。非 `volatile` 局部变量在 `longjmp` 后的值是未定义的——可能被回滚到 `setjmp` 时的值，
也可能保持修改后的值，取决于编译器优化。加 `volatile` 可以保证变量的值是 `longjmp` 调用时的值。
规则：如果 `setjmp` 和 `longjmp` 之间修改的变量需要在 `longjmp` 后保持值，必须加 `volatile`。
</details>

<details><summary>3. HFT 进程如何实现优雅退出？</summary>

注册 `SIGINT`/`SIGTERM` 处理器，处理器只设置 `volatile sig_atomic_t should_stop = 1`。
主循环每次迭代检查 `should_stop`，为真时退出循环，在正常上下文中做清理（关闭连接、保存状态、释放资源）。
不在信号处理器中做任何复杂操作。这样保证了：① 信号处理器安全（async-signal-safe）；
② 清理在正常上下文中进行（可以用任何函数）；③ 退出过程可控（主循环决定何时退出）。
</details>
