/* c6_5_stack_frame.c — 6.5 栈与栈帧：方向、每帧字节数、栈上限、爆栈后果
 * 编译: gcc -O2 -Wall -Wextra -o c6_5_stack_frame c6_5_stack_frame.c
 * 说明: 递归函数里留 256B padding 并真实写它，防止被优化成尾调用/循环。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define PAD         256
#define SAFE_DEPTH  8000        /* 8000 × 272B ≈ 2.1MB，稳在 8MB 栈以内 */

static volatile long depth = 0;
static volatile long top1 = 0, top2 = 0;

/* 有深度上限：用来量每帧占用 */
static void recurse_safe(void)
{
    volatile char pad[PAD];
    pad[0] = (char)depth;                        /* 真实使用，防优化掉 */
    if (!top1) top1 = (long)&pad[0];
    else if (!top2) top2 = (long)&pad[0];

    depth++;
    if (depth < SAFE_DEPTH) recurse_safe();
    pad[PAD - 1] = 1;                            /* 递归调用后仍有代码 -> 不是尾调用 */
}

/* 无深度上限：制造爆栈 */
static void recurse_bomb(void)
{
    volatile char pad[PAD];
    pad[0] = (char)depth;
    depth++;
    recurse_bomb();
    pad[PAD - 1] = 1;
}

int main(void)
{
    /* ---- A. 栈增长方向 ---- */
    char a = 0, b = 0;
    char *hi = &a, *lo = &b;
    if (hi < lo) { char *t = hi; hi = lo; lo = t; }
    printf("A. 先声明的 %p > 后声明的 %p  ->  x86-64 栈向【低地址】增长\n",
           (void *)hi, (void *)lo);

    /* ---- B. 每帧字节数 ---- */
    top1 = top2 = 0; depth = 0;
    recurse_safe();
    long frame = top1 - top2;
    printf("B. 递归深度 %ld，相邻两层 pad[0] 相距 %ld B -> 每帧约 %ld B\n",
           depth, frame, frame);
    printf("   （8MB 栈 ÷ %ld B/帧 ≈ %ld 层就会耗尽）\n",
           frame, (8L * 1024 * 1024) / frame);

    /* ---- C. 栈上限 ---- */
    struct rlimit rl;
    getrlimit(RLIMIT_STACK, &rl);
    printf("C. RLIMIT_STACK = %lu MB（ulimit -s 改的就是它）\n",
           (unsigned long)(rl.rlim_cur / 1024 / 1024));

    /* ---- D. 爆栈后果：放进子进程，避免把自己搞死 ---- */
    pid_t c = fork();
    if (c == 0) { depth = 0; recurse_bomb(); _exit(0); }
    int st = 0;
    wait(&st);
    if (WIFSIGNALED(st))
        printf("D. 无上限递归的子进程：被信号 %d 杀死（%s）—— 栈溢出不是「优雅报错」，是直接 SIGSEGV\n",
               WTERMSIG(st), WTERMSIG(st) == SIGSEGV ? "SIGSEGV" : "其他");
    return 0;
}
