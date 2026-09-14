/* TLPI 第 10 章 §10.6 The Software Clock (Jiffies)  —— 本 demo 是本章最该看的一个
 * clock_getres 到底报多少？原书说「1/HZ」，现代内核还这样吗？
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_6_clockres c10_6_clockres.c
 * 运行: ./c10_6_clockres                      (无需参数、无需权限；跑一会儿)
 *
 * ⭐ 结论先给：原书（2010 年，2.6 内核）说 CLOCK_REALTIME / CLOCK_MONOTONIC 的
 *    分辨率是 1/HZ（HZ=250 时 4 ms）。**在启用了高分辨率定时器（hrtimer）的
 *    现代内核上，这两个时钟报的是 1 ns**；只有 *_COARSE 那两个才还是 1/HZ。
 *    本 demo 把这两件事都现场量出来。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void show_clock_getres(clockid_t id, const char *name, long tck)
{
    struct timespec res;
    long ns;

    errno = 0;
    if (clock_getres(id, &res) != 0) {
        printf("  %-24s clock_getres 失败: %s\n", name, strerror(errno));
        return;
    }
    ns = (long) res.tv_sec * 1000000000L + res.tv_nsec;
    printf("  %-24s res = %ld.%09ld s = %9ld ns  %s\n",
           name, (long) res.tv_sec, res.tv_nsec, ns,
           (ns == 1000000000L / tck) ? "<-- 正好 1/HZ（jiffy 粒度）"
                                     : (ns == 1 ? "<-- 1 ns（hrtimer）" : ""));
}

/* 连续两次读同一个时钟，统计「有没有前进」以及「最小正增量是多少 ns」。
   COARSE 时钟在一个 jiffy 内根本不动 → 会出现大量 0 增量。 */
static void probe_granularity(clockid_t id, const char *name, long iters)
{
    struct timespec a, b;
    long best = -1, changed = 0;

    for (long i = 0; i < iters; i++) {
        clock_gettime(id, &a);
        clock_gettime(id, &b);
        long d = (long) (b.tv_sec - a.tv_sec) * 1000000000L
               + (long) (b.tv_nsec - a.tv_nsec);
        if (d != 0) {
            changed++;
            if (best < 0 || d < best)
                best = d;
        }
    }
    printf("  %-24s %ld 次连续读里 %8ld 次有前进   最小正增量 = %ld ns\n",
           name, iters, changed, best);
}

int main(void)
{
    long tck = sysconf(_SC_CLK_TCK);

    printf("== 0. 三个很容易混的「每秒几个 tick」 ==\n");
    printf("sysconf(_SC_CLK_TCK) = %ld        <- USER_HZ：内核写死的 100，\n", tck);
    printf("                                     **不是** CONFIG_HZ！第 4 段会现场撞上这个坑\n");
    printf("CLOCKS_PER_SEC       = %-8ld <- clock(3) 的单位，又是另一个数\n",
           (long) CLOCKS_PER_SEC);
    printf("CLOCK_{REALTIME,MONOTONIC}_COARSE  <- 真正的 1/CONFIG_HZ，第 1 段直接量\n");
    printf("\n");

    printf("== 1. clock_getres(): 每个时钟自己报的分辨率 ==\n");
    show_clock_getres(CLOCK_REALTIME, "CLOCK_REALTIME", tck);
    show_clock_getres(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", tck);
    show_clock_getres(CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW", tck);
    show_clock_getres(CLOCK_REALTIME_COARSE, "CLOCK_REALTIME_COARSE", tck);
    show_clock_getres(CLOCK_MONOTONIC_COARSE, "CLOCK_MONOTONIC_COARSE", tck);
    show_clock_getres(CLOCK_BOOTTIME, "CLOCK_BOOTTIME", tck);
    show_clock_getres(CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID", tck);
    show_clock_getres(CLOCK_THREAD_CPUTIME_ID, "CLOCK_THREAD_CPUTIME_ID", tck);
    printf("\n");

    printf("== 2. 实际能分辨多少：连续两次读的最小正增量 ==\n");
    probe_granularity(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", 2000000);
    probe_granularity(CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW", 2000000);
    probe_granularity(CLOCK_REALTIME, "CLOCK_REALTIME", 2000000);
    probe_granularity(CLOCK_MONOTONIC_COARSE, "CLOCK_MONOTONIC_COARSE", 2000000);
    probe_granularity(CLOCK_REALTIME_COARSE, "CLOCK_REALTIME_COARSE", 2000000);
    printf("\n");

    printf("== 3. 把时钟值本身摊开看 ==\n");
    {
        struct timespec ts;

        clock_gettime(CLOCK_REALTIME, &ts);
        printf("  REALTIME  = %ld.%09ld\n", (long) ts.tv_sec, (long) ts.tv_nsec);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("  MONOTONIC = %ld.%09ld   <- 起点未定义，通常是开机时刻\n",
               (long) ts.tv_sec, (long) ts.tv_nsec);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        printf("  RAW       = %ld.%09ld\n", (long) ts.tv_sec, (long) ts.tv_nsec);
        clock_gettime(CLOCK_BOOTTIME, &ts);
        printf("  BOOTTIME  = %ld.%09ld\n", (long) ts.tv_sec, (long) ts.tv_nsec);
        clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
        /* ⚠️ 别以为 COARSE 的值是 jiffy 的整数倍 —— 实测看着不像。它只是
           「一个 jiffy 之内不变」（见第 2 段的最小正增量 ≈ 1 ms），
           具体取值不对齐任何边界。 */
        printf("  COARSE    = %ld.%09ld   <- 一个 jiffy 内不变，但取值不对齐边界\n",
               (long) ts.tv_sec, (long) ts.tv_nsec);
    }
    printf("\n");

    printf("== 4. 结论：原书的「1/HZ」在今天要拆成两个数 ==\n");
    {
        struct timespec res;
        long coarse_ns = 0, from_tck = 1000000000L / tck;

        if (clock_getres(CLOCK_MONOTONIC_COARSE, &res) == 0)
            coarse_ns = (long) res.tv_sec * 1000000000L + res.tv_nsec;

        printf("  原书 §10.6（2010 年，2.6 内核）的表述是「分辨率 = 1/HZ」。本机实测：\n");
        printf("    CLOCK_REALTIME / MONOTONIC / RAW / BOOTTIME / *_CPUTIME_ID\n");
        printf("        -> 1 ns。它们走 hrtimer，内核把时间源直接换算到纳秒，HZ 管不着。\n");
        printf("    CLOCK_{REALTIME,MONOTONIC}_COARSE\n");
        printf("        -> %ld ns。这两个仍然被 tick 限住。\n\n", coarse_ns);

        printf("  ⚠️ 但**不能**用 sysconf(_SC_CLK_TCK) 去推它：\n");
        printf("     _SC_CLK_TCK = %ld -> 推算 %ld ns，实测 %ld ns，差了 %ld 倍。\n",
               tck, from_tck, coarse_ns, coarse_ns ? from_tck / coarse_ns : 0);
        printf("     原因是 _SC_CLK_TCK 给的**不是 CONFIG_HZ**，而是写死的 USER_HZ=100：\n");
        printf("       内核把 AT_CLKTCK 塞进 auxv 时写的是 CLOCKS_PER_SEC，\n");
        printf("       CLOCKS_PER_SEC 又等于 USER_HZ（include/asm-generic/param.h:10）。\n");
        printf("       COARSE 的粒度才是真 1/CONFIG_HZ —— 本容器 CONFIG_HZ = %ld。\n\n",
               coarse_ns ? 1000000000L / coarse_ns : 0);

        printf("  所以「软件时钟粒度限制一切」只对**非 hrtimer 路径**成立：\n");
        printf("    定时器接口（nanosleep / timer_settime）走 hrtimer，可以远小于 1 jiffy\n");
        printf("    （见 ex10_3：请求 1 us 的 nanosleep 确实能醒来）。\n");
    }
    return 0;
}
