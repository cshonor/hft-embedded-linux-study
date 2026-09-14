/* TLPI 第 10 章 §10.9 练习 10-1（原书唯一的一道习题）
 *
 * 题干（原书 §10.9，逐字）：
 *   "Assume a system where the value returned by the call sysconf(_SC_CLK_TCK)
 *    is 100. Assuming that the clock_t value returned by times() is an unsigned
 *    32-bit integer, how long will it take before this value cycles so that it
 *    restarts at 0? Perform the same calculation for the CLOCKS_PER_SEC value
 *    returned by clock()."
 *
 * 中译：假设 sysconf(_SC_CLK_TCK) 返回 100；再假设 times() 返回的 clock_t
 * 是**无符号 32 位**整数，那么这个值回绕到 0 需要多久？对 clock() 返回的
 * CLOCKS_PER_SEC 值做同样的计算。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex10_1_clock_cycle ex10_1_clock_cycle.c
 * 运行: ./ex10_1_clock_cycle                    (无需参数、无需权限)
 *
 * 这道题真正要你发现的是：**times() 和 clock() 的时钟频率不一样**，
 * 所以同样宽度的 clock_t，回绕周期差了两个数量级。
 * 顺带把「假想 32 位」与「本机真实 clock_t」对照一下。
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

static void human(double secs, char *out, size_t n)
{
    if (secs >= 365.0 * 24 * 3600)
        snprintf(out, n, "%.2f 年", secs / (365.0 * 24 * 3600));
    else if (secs >= 24 * 3600)
        snprintf(out, n, "%.2f 天", secs / (24 * 3600));
    else if (secs >= 3600)
        snprintf(out, n, "%.4f 小时", secs / 3600);
    else if (secs >= 60)
        snprintf(out, n, "%.2f 分钟", secs / 60);
    else
        snprintf(out, n, "%.3f 秒", secs);
}

int main(void)
{
    long tck = sysconf(_SC_CLK_TCK);
    char h1[40], h2[40], h3[40];
    double cycle_times, cycle_clock, cycle_real;

    printf("== 0. 本机的三个常数 ==\n");
    printf("sysconf(_SC_CLK_TCK) = %ld      <- times() 的每秒 tick 数\n", tck);
    printf("CLOCKS_PER_SEC       = %ld  <- clock() 的每秒 tick 数\n",
           (long) CLOCKS_PER_SEC);
    printf("sizeof(clock_t)      = %zu 字节   <- 本题前提说它是 32 位\n\n",
           sizeof(clock_t));

    /* --- 题干的假设：clock_t 是无符号 32 位 --- */
    {
        double max32 = 4294967296.0;      /* 2^32，回绕点的值就是它 */

        cycle_times = max32 / (double) tck;
        cycle_clock = max32 / (double) CLOCKS_PER_SEC;
        human(cycle_times, h1, sizeof h1);
        human(cycle_clock, h2, sizeof h2);

        printf("== 1. 按题干的假设算（clock_t = 无符号 32 位，2^32 = %.0f）==\n", max32);
        printf("  times() 按 _SC_CLK_TCK=%ld 计：\n", tck);
        printf("    %.0f / %ld = %.2f 秒 = %s\n", max32, tck, cycle_times, h1);
        printf("  clock() 按 CLOCKS_PER_SEC=%ld 计：\n", (long) CLOCKS_PER_SEC);
        printf("    %.0f / %ld = %.4f 秒 = %s\n", max32, (long) CLOCKS_PER_SEC,
               cycle_clock, h2);
        printf("  → 同样的位宽，频率越高回绕越快，两者差了 %.0f 倍\n\n",
               (double) CLOCKS_PER_SEC / (double) tck);
    }

    /* --- 本机真实情况 --- */
    printf("== 2. 本机真实情况（clock_t 其实是 %zu 字节的有符号整数）==\n", sizeof(clock_t));
    {
        int bits = (int) (sizeof(clock_t) * 8);

        printf("  clock_t 是 %d 位有符号 -> 上界 %.0f\n",
               bits, (double) ((uint64_t) 1 << (bits - 1)) - 1);
        cycle_real = ((double) ((uint64_t) 1 << (bits - 1)) - 1) / (double) tck;
        human(cycle_real, h3, sizeof h3);
        printf("  times() 的回绕周期 = %.0f / %ld = %.3e 秒 = %s\n",
               ((double) ((uint64_t) 1 << (bits - 1)) - 1), tck, cycle_real, h3);
        printf("  → 在这台机器上「回绕」不是现实问题；题干里的 32 位是**教学假设**，\n");
        printf("    也是嵌入式老平台（32 位 time_t + 32 位 clock_t）的真实处境。\n\n");
    }

    printf("== 3. 现场看一眼真实的 tick 值 ==\n");
    {
        struct tms t;
        clock_t c;

        if (times(&t) == (clock_t) -1) {
            printf("  times() 失败\n");
            return 1;
        }
        c = clock();
        printf("  times() 返回值（开机以来总 tick）= %ld\n", (long) times(&t));
        printf("    tms_utime=%ld tms_stime=%ld tms_cutime=%ld tms_cstime=%ld\n",
               (long) t.tms_utime, (long) t.tms_stime,
               (long) t.tms_cutime, (long) t.tms_cstime);
        printf("  clock() 返回值（本进程 CPU tick）= %ld\n", (long) c);
        printf("  → times() 的第一个返回值是 **开机以来的 clock_t 计数**，\n");
        printf("     不是进程时间；进程时间在 struct tms 里。这是这道题的隐藏考点。\n");
    }
    return 0;
}
