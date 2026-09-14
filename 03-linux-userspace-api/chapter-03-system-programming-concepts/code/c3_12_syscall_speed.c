/* TLPI 第 03 章 §3.7（成本全景）—— 系统调用到底贵多少：复刻原书 progconc/syscall_speed.c
 *
 * 本 demo 给 §3.7 小结做量化收束：把「系统调用 / glibc 包装 / 普通函数 / 纯用户态库函数」
 * 四种调用的单次耗时并排量出来，说明「贵的是跨特权级，不是库函数」。
 *
 * 原书这个程序（无 Listing 编号，属「supplementary file」）的做法：
 *   用 getppid() 与 syscall(SYS_getppid) 各跑 N 次，比较耗时，证明
 *   「glibc 包装 + 真正陷入内核」比普通函数调用贵一到两个数量级。
 *
 * 本 demo 在原书基础上加了两条对照：
 *   ③ 普通函数调用（同一进程内，无特权级切换）—— 作为「零点」
 *   ④ 纯用户态库函数调用（strlen 这种也被编译成函数调用）—— 验证「库函数也是零成本」
 *
 * 计时用 CLOCK_MONOTONIC：它只受 NTP 频率微调（adjtime）影响，不会被墙上时钟回拨搞乱。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_12 c3_12_syscall_speed.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define ITERS 2000000L

static volatile long sink;    /* 防止优化把整个循环删掉 */

static long nop_call(long x)  /* 普通函数调用：不陷入内核 */
{
    return x + 1;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static void bench(const char *tag, long (*fn)(long), long arg)
{
    double t0 = now_sec();
    long acc = 0;
    for (long i = 0; i < ITERS; i++) {
        acc += fn(arg);
    }
    double dt = now_sec() - t0;
    sink = acc;
    printf("  %-30s %7.1f ms  →  %6.1f ns/次\n", tag, dt * 1e3, dt / (double) ITERS * 1e9);
}

/* ---- 四种被测目标，签名统一成 long(long) 便于复用同一个 bench() ---- */

static long f_nop(long x)                 /* ③ 普通函数调用 */
{
    return nop_call(x);
}

static long f_userlib(long x)             /* ④ 纯用户态库函数（无内联，-O0 下是真调用） */
{
    char buf[16];
    snprintf(buf, sizeof buf, "%ld", x & 1);
    return buf[0];
}

static long f_glibc_wrapper(long x)       /* ① glibc 包装的 getppid() */
{
    (void) x;
    return (long) getppid();
}

static long f_syscall2(long x)            /* ② syscall(2) 通用入口 */
{
    (void) x;
    return (long) syscall(SYS_getppid);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== 系统调用开销实测（%ld 次，CE 容器 / gcc 13.3 / x86-64）===\n", ITERS);
    printf("  %-30s %7s     %8s\n", "被测对象", "总耗时", "单次");

    bench("① glibc 包装 getppid()", f_glibc_wrapper, 1);
    bench("② syscall(SYS_getppid)", f_syscall2, 1);
    bench("③ 普通函数调用 nop_call()", f_nop, 1);
    bench("④ 纯用户态库函数 snprintf()", f_userlib, 1);

    printf("\n=== 怎么读这张表 ===\n");
    printf("  · ①和②都在「真正陷入内核」那一档；两者的差距 = syscall(2) 自己套的那层壳\n");
    printf("    （存参、查表、把 -errno 翻成 -1+errno），不涉及特权级切换\n");
    printf("  · ③是零点：同一特权级内的 call/ret\n");
    printf("  · ④说明「库函数」本身不贵，贵的是**跨特权级**；atoi/strlen/snprintf 都是用户态\n");
    printf("  · 单次纳秒数是**这台机器**的绝对值，不能当你的机器/你的板的数值；\n");
    printf("    可迁移的结论只有**比值**：系统调用约比普通调用贵一到两个数量级\n");

    printf("\n=== 对 HFT / 嵌入式的直接推论 ===\n");
    printf("  1. 热路径别逐字节 read/write：一次批量调用替代 N 次小调用\n");
    printf("  2. 能用 vDSO 的调用（clock_gettime/gettimeofday/time/getcpu）不陷入内核\n");
    printf("  3. 同机通信优先 AF_UNIX/共享内存，省掉整个网络栈的系统调用序列\n");
    printf("  4. 别用 gettimeofday 之外的「看起来免费」的库函数做高频日志：printf 的代价在\n");
    printf("     格式化（用户态）而不是 write，但它会写很多字节 → 仍要批量化\n");
    return 0;
}
