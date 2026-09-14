/*
 * c2_2_backtrace.c —— 「崩溃现场」长什么样：用 glibc backtrace() 打出真实调用栈
 *
 * 用途：Ch2 的核心工具是 gdb，但 gdb 的 `bt` 命令底层就是**遍历栈帧**。
 *       这份程序用 glibc 的 backtrace() / backtrace_symbols_fd() 在**进程内**
 *       把同一份信息打出来 —— 于是「调用栈长什么样」不靠想象，可以真跑真看。
 *
 *       ★ 关键实验：同一份源码，-O0 与 -O2 打出的**帧数不一样**。
 *         这就是 2.7「debug 版 vs release 版」最直观的证据：
 *         优化把中间函数内联掉了，栈帧消失，gdb 的 bt 也就跟着变浅。
 *
 * 用法：./c2_2_backtrace
 *
 * 编译（要求符号名可解析，-rdynamic 必须加）：
 *   gcc -g -O0 -rdynamic -Wall -Wextra -o c2_2_bt_O0 c2_2_backtrace.c
 *   gcc -g -O2 -rdynamic -Wall -Wextra -o c2_2_bt_O2 c2_2_backtrace.c
 *
 * 实测（gcc 13.3.0）见 Ch2 笔记；要点：-O0 能看到 6 帧，-O2 明显变浅。
 */
#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BT_DEPTH 32

/* 收到 SIGSEGV 时，在进程内打印一份「穷人版 bt」 */
static void on_crash(int sig)
{
    void  *frames[BT_DEPTH];
    int    n = backtrace(frames, BT_DEPTH);
    char   hdr[128];

    /* ⚠️ 信号处理函数里只能用 async-signal-safe 函数：
     *    printf / malloc 都可能死锁，write() / backtrace_symbols_fd() 才是安全的。 */
    int len = snprintf(hdr, sizeof hdr,
                       "\n--- 捕获信号 %d(%s)，进程内 backtrace 共 %d 帧 ---\n",
                       sig, strsignal(sig), n);
    if (len > 0)
        (void)!write(STDERR_FILENO, hdr, (size_t)len);

    /* _fd 版本直接写 fd，不分配内存（backtrace_symbols 会 malloc，不安全） */
    backtrace_symbols_fd(frames, n, STDERR_FILENO);

    _exit(128 + sig);           /* 用 _exit 而非 exit，避免跑 atexit/刷新 stdio */
}

/* ---- 一串刻意分层、方便观察栈帧的调用链 ---- */
static int layer_d(int x)
{
    int *p = NULL;
    printf("layer_d: 准备解引用 NULL（x=%d）\n", x);
    fflush(stdout);
    *p = x;                     /* ← SIGSEGV */
    return *p;
}

static int layer_c(int x) { return layer_d(x + 1); }
static int layer_b(int x) { return layer_c(x + 1); }
static int layer_a(int x) { return layer_b(x + 1); }

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_crash;
    sigemptyset(&sa.sa_mask);
    /* SA_NODEFER：处理 SIGSEGV 时不要屏蔽它自己，方便调试 */
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    printf("调用链 main → layer_a → layer_b → layer_c → layer_d → 崩溃\n");
    fflush(stdout);
    return layer_a(0);
}
