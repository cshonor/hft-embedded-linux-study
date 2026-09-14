/* TLPI 第 10 章 §10.6 The Software Clock (Jiffies)（开销一侧）
 * 「读一次时间」到底多贵：六条读法的单次耗时对照
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_6_coarse_cost c10_6_coarse_cost.c
 * 运行: ./c10_6_coarse_cost                    (无需参数；跑几秒)
 *
 * ⚠️ 必须 -O0：本 demo 要量的是「函数调用真的发生」的代价，-O2 会把
 *    nop_call() 和好几处读时钟全部内联/消除掉，测出来是 0。
 *
 * 要看的结论：
 *   * time() / gettimeofday() / clock_gettime(REALTIME 或 MONOTONIC) 都走
 *     **vDSO**（用户态直接读内核共享页），所以都在几十 ns 量级，**不陷内核**；
 *   * 但 *_COARSE 比对应的精确版更便宜 —— 它连时间源都不用换算，直接取
 *     上次 jiffy 更新时缓存下来的值；
 *   * nop_call() 是基线（纯循环 + 一次空调用），任何一条读法减去它就得到净开销。
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define N 2000000L

/* 基线：一个保证不被内联、且不产生任何副作用的空函数 */
static __attribute__((noinline)) void nop_call(void)
{
    __asm__ __volatile__("");
}

/* 把「取两个 MONOTONIC 读数」的开销从各测里扣掉 —— 方法是用同一套
   计时框架包住空循环体，只比总时长。 */

static double per_call_clock_gettime(clockid_t id, long n, volatile long *sink)
{
    struct timespec a, b, tmp;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < n; i++) {
        clock_gettime(id, &tmp);
        *sink += tmp.tv_nsec;
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double) ((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec)) / (double) n;
}

static double per_call_gettimeofday(long n, volatile long *sink)
{
    struct timespec a, b;
    struct timeval tv;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < n; i++) {
        gettimeofday(&tv, NULL);
        *sink += tv.tv_usec;
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double) ((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec)) / (double) n;
}

static double per_call_time(long n, volatile long *sink)
{
    struct timespec a, b;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < n; i++) {
        time_t t = time(NULL);
        *sink += (long) t;
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double) ((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec)) / (double) n;
}

static double per_call_nop(long n, volatile long *sink)
{
    struct timespec a, b;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < n; i++) {
        nop_call();
        *sink += 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double) ((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec)) / (double) n;
}

#if defined(__x86_64__)
static double per_call_rdtsc(long n, volatile long *sink)
{
    struct timespec a, b;

    clock_gettime(CLOCK_MONOTONIC, &a);
    for (long i = 0; i < n; i++) {
        uint32_t lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        *sink += (long) ((uint64_t) hi << 32 | lo) & 0xffffL;
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (double) ((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec)) / (double) n;
}
#endif

int main(void)
{
    volatile long sink = 0;
    double base, v;

    printf("每组 %ld 次调用，-O0 编译（防止内联/消除）\n\n", N);

    base = per_call_nop(N, &sink);
    printf("  %-34s %8.1f ns/call   <- 基线：纯循环 + 一次空函数调用\n",
           "nop_call()", base);

    v = per_call_time(N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "time(NULL)", v, v - base);

    v = per_call_gettimeofday(N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "gettimeofday()", v, v - base);

    v = per_call_clock_gettime(CLOCK_REALTIME, N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "clock_gettime(REALTIME)", v, v - base);

    v = per_call_clock_gettime(CLOCK_MONOTONIC, N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "clock_gettime(MONOTONIC)", v, v - base);

    v = per_call_clock_gettime(CLOCK_MONOTONIC_RAW, N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "clock_gettime(MONOTONIC_RAW)", v, v - base);

    v = per_call_clock_gettime(CLOCK_REALTIME_COARSE, N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "clock_gettime(REALTIME_COARSE)", v, v - base);

    v = per_call_clock_gettime(CLOCK_MONOTONIC_COARSE, N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "clock_gettime(MONOTONIC_COARSE)", v, v - base);

#if defined(__x86_64__)
    v = per_call_rdtsc(N, &sink);
    printf("  %-34s %8.1f ns/call   净 %+8.1f ns\n", "rdtsc 指令", v, v - base);
#endif

    (void) sink;
    printf("\n三条要点：\n");
    printf("  1) time()/gettimeofday()/clock_gettime() 全是几十 ns 量级 —— 它们走\n");
    printf("     vDSO，是**用户态**读内核共享页，没有 syscall 指令、没有特权级切换；\n");
    printf("  2) COARSE 变体最便宜：它不碰硬件时间源，直接返回上个 jiffy 缓存的值，\n");
    printf("     代价是精度掉到 1/HZ；\n");
    printf("  3) rdtsc 是最便宜的「高分辨率」读法（一条指令），但它的单位是 CPU 周期，\n");
    printf("     要自己用频率换算，且跨核/变频下不一定可比 —— 所以 HFT 里\n");
    printf("     「测延迟用 MONOTONIC，测开销用 rdtsc」是常见分工。\n");
    return 0;
}
