/* TLPI 第 2 章 §2.16 —— 日期与时间：先把「哪一类时钟」分清楚
 *
 * 编译：gcc -O0 -Wall -Wextra c2_16_clock.c -o c2_16
 * 运行：./c2_16
 *
 * 本节要钉死的事实：
 *   ① 时钟不是一个，是一族：墙上时间（可被 NTP 改）与单调时间（只前进）必须分开。
 *   ② 测「过了多久」只能用 CLOCK_MONOTONIC / CLOCK_BOOTTIME，绝不能用 REALTIME。
 *   ③ 进程/线程 CPU 时间（CLOCK_PROCESS_CPUTIME_ID）与墙上时间是两回事。
 *   ④ /proc/uptime 与 CLOCK_BOOTTIME 同源。
 *
 * ⚠️ 本节的定位是「分清楚有哪几类时钟」；struct tm / strftime / 时区 / 夏令时
 *    这套日历转换在 TLPI Ch10 展开，这里不重复。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static long long ns_diff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec);
}

int main(void)
{
    printf("=== ① 一族时钟的当前值与分辨率 ===\n");
    struct { clockid_t id; const char *name; const char *meaning; } clocks[] = {
        { CLOCK_REALTIME,           "CLOCK_REALTIME",           "墙上时间：可被 NTP/adjtime 改，会跳" },
        { CLOCK_MONOTONIC,          "CLOCK_MONOTONIC",          "单调：只前进，含 suspend 以外的停机时间" },
        { CLOCK_BOOTTIME,           "CLOCK_BOOTTIME",           "单调 + 含 suspend 时间" },
        { CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID", "本进程消耗的 CPU 时间" },
        { CLOCK_THREAD_CPUTIME_ID,  "CLOCK_THREAD_CPUTIME_ID",  "本线程消耗的 CPU 时间" },
    };
    printf("  %-24s %-20s %-12s %s\n", "clock", "now", "res", "含义");
    printf("  %-24s %-20s %-12s %s\n", "------------------------",
           "--------------------", "------------", "------------------");
    for (unsigned i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++) {
        struct timespec ts, res;
        char val[32] = "?", rv[32] = "?";
        if (clock_gettime(clocks[i].id, &ts) == 0)
            snprintf(val, sizeof(val), "%lld.%09ld", (long long)ts.tv_sec, ts.tv_nsec);
        if (clock_getres(clocks[i].id, &res) == 0)
            snprintf(rv, sizeof(rv), "%lldns", (long long)res.tv_sec * 1000000000LL + res.tv_nsec);
        printf("  %-24s %-20s %-12s %s\n", clocks[i].name, val, rv, clocks[i].meaning);
    }
    printf("  -> CLOCK_REALTIME 是一千七百多万秒（epoch 起算）；\n");
    printf("     CLOCK_MONOTONIC 是「开机以来的秒数」，明显更小 —— 两者不同源。\n");

    printf("\n=== ② 用哪个时钟测耗时：一个不能省的实验 ===\n");
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    volatile long acc = 0;
    for (long i = 0; i < 2000000; i++) acc += i;        /* 一段假热路径 */
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long long mono_ns = ns_diff(t0, t1);

    struct timespec w0, w1;
    clock_gettime(CLOCK_REALTIME, &w0);
    struct timespec c0, c1;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &c0);

    volatile long acc2 = 0;
    for (long i = 0; i < 2000000; i++) acc2 += i;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &c1);
    clock_gettime(CLOCK_REALTIME, &w1);

    printf("  同一段循环跑两遍：\n");
    printf("    第 1 遍  MONOTONIC 测得        = %lld ns\n", mono_ns);
    printf("    第 2 遍  REALTIME  测得        = %lld ns\n", ns_diff(w0, w1));
    printf("    第 2 遍  进程 CPU 时间测得     = %lld ns\n", ns_diff(c0, c1));
    printf("  -> 墙钟（REALTIME/MONOTONIC）会包含「被别的进程挤掉 CPU」的时间；\n");
    printf("     进程 CPU 时间只算自己真正占着 CPU 的那部分。\n");
    printf("  -> 三者都不该用 REALTIME 来做区间测量：NTP 一调整就是负数区间。\n");

    printf("\n=== ③ REALTIME 与 MONOTONIC 的差值会漂移 ===\n");
    struct timespec r1, m1, r2, m2;
    clock_gettime(CLOCK_REALTIME, &r1); clock_gettime(CLOCK_MONOTONIC, &m1);
    struct timespec nap = { 0, 20000000L };             /* 20ms */
    nanosleep(&nap, NULL);
    clock_gettime(CLOCK_REALTIME, &r2); clock_gettime(CLOCK_MONOTONIC, &m2);
    printf("  REALTIME  在这段里走了 %lld ns\n", ns_diff(r1, r2));
    printf("  MONOTONIC 在这段里走了 %lld ns\n", ns_diff(m1, m2));
    printf("  两者基数之差 = %lld ns  （≈ 一个巨大的常数：epoch 到开机那一刻）\n",
           ns_diff(m2, r2));
    printf("  -> 差值本身没有意义，有意义的是「两个时钟各自前进的速度是否一致」。\n");

    printf("\n=== ④ 传统接口与粗糙时钟 ===\n");
    printf("  time(NULL)               = %ld  （秒级，最省）\n", (long)time(NULL));
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0)
        printf("  gettimeofday(&tv)        = %ld.%06ld\n", (long)tv.tv_sec, (long)tv.tv_usec);
    struct timespec coarse;
    if (clock_gettime(CLOCK_REALTIME_COARSE, &coarse) == 0)
        printf("  CLOCK_REALTIME_COARSE    = %lld.%09ld  （走缓存，比精确的便宜得多）\n",
               (long long)coarse.tv_sec, coarse.tv_nsec);
    printf("  -> 只想知道「大概几点了」用 COARSE 或 time()；\n");
    printf("     要测间隔才需要精确的 MONOTONIC。\n");

    printf("\n=== ⑤ /proc 里能佐证的两个文件 ===\n");
    FILE *f = fopen("/proc/uptime", "r");
    if (f) {
        double up = 0, idle = 0;
        if (fscanf(f, "%lf %lf", &up, &idle) == 2) {
            printf("  /proc/uptime 第一列 = %.2f 秒（≈ CLOCK_BOOTTIME）\n", up);
            printf("  /proc/uptime 第二列 = %.2f 秒（所有 CPU 的累计 idle 时间）\n", idle);
        }
        fclose(f);
    }
    errno = 0;
    const char *tz = getenv("TZ");
    printf("  TZ 环境变量 = %s\n", tz ? tz : "(未设置 → 用系统默认，通常是 UTC)");
    printf("  localtime()/strftime()/时区/夏令时 → 见 TLPI Ch10，本节不展开。\n");
    return 0;
}
