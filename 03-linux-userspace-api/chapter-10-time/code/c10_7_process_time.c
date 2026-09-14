/* TLPI 第 10 章 §10.7 Process Time
 * 进程时间：times(2) 的四个字段 / clock(3) / CLOCK_PROCESS_CPUTIME_ID /
 * CLOCK_THREAD_CPUTIME_ID / getrusage 的两个 RUSAGE_*
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_7_process_time c10_7_process_time.c
 * 运行: ./c10_7_process_time                   (无需参数；跑一两秒)
 *
 * ⭐ 关键区分：
 *     墙上时间(wall)  —— CLOCK_MONOTONIC，睡 100 ms 就涨 100 ms；
 *     进程 CPU 时间   —— times()/clock()/CLOCK_PROCESS_CPUTIME_ID，睡 100 ms
 *                        期间进程没在跑，**一点都不涨**。
 *   然后 times() 的粒度是 1/HZ，所以「很小的循环」会读到 0 tick ——
 *   这不是没耗 CPU，是刻度太粗。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static long tck;
static struct timespec wall0;

static double wall_now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) (ts.tv_sec - wall0.tv_sec)
         + (double) (ts.tv_nsec - wall0.tv_nsec) / 1e9;
}

static double cpu_now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static void snapshot(const char *label)
{
    struct tms t;
    clock_t c = clock();

    if (times(&t) == (clock_t) -1) {
        printf("times() 失败\n");
        return;
    }
    printf("  %-22s wall=%6.3f s  cpu=%6.3f s | "
           "utime=%5.2f stime=%5.2f cutime=%5.2f cstime=%5.2f | "
           "clock()=%.3f s | ticks=%ld/%ld/%ld/%ld\n",
           label, wall_now(), cpu_now(),
           (double) t.tms_utime / tck, (double) t.tms_stime / tck,
           (double) t.tms_cutime / tck, (double) t.tms_cstime / tck,
           (double) c / CLOCKS_PER_SEC,
           (long) t.tms_utime, (long) t.tms_stime,
           (long) t.tms_cutime, (long) t.tms_cstime);
}

int main(void)
{
    clock_gettime(CLOCK_MONOTONIC, &wall0);
    tck = sysconf(_SC_CLK_TCK);

    printf("== 0. 时间刻度的三个常数 ==\n");
    printf("sysconf(_SC_CLK_TCK) = %ld   <- times() 的单位：每秒 %ld 个 tick\n", tck, tck);
    printf("CLOCKS_PER_SEC       = %ld   <- clock() 的单位，和上面**不是一个数**\n",
           (long) CLOCKS_PER_SEC);
    printf("sizeof(clock_t)      = %zu\n\n", sizeof(clock_t));

    printf("== 1. 各阶段的时间账 ==\n");
    snapshot("启动");

    /* (a) 纯用户态忙等：utime 应涨、stime 不该涨 */
    {
        volatile unsigned long long s = 0;
        for (long i = 0; i < 20000000L; i++)
            s += (unsigned long long) i;
        (void) s;
    }
    snapshot("纯用户态忙等后");

    /* (b) 系统调用密集：stime 应涨 */
    {
        long j;
        for (j = 0; j < 3000000L; j++)
            (void) getppid();
    }
    snapshot("300 万次 getppid");

    /* (c) 睡眠：wall 涨、cpu 不涨 */
    {
        struct timespec req = { 0, 300 * 1000 * 1000 };   /* 300 ms */

        nanosleep(&req, NULL);
    }
    snapshot("nanosleep(300ms)");

    printf("\n");
    printf("== 2. CLOCK_PROCESS_CPUTIME_ID vs CLOCK_THREAD_CPUTIME_ID ==\n");
    {
        struct timespec p, t;

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &p);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
        printf("  单线程进程里两者几乎相同：\n");
        printf("    进程 CPU = %ld.%09ld s\n", (long) p.tv_sec, (long) p.tv_nsec);
        printf("    线程 CPU = %ld.%09ld s\n", (long) t.tv_sec, (long) t.tv_nsec);
        printf("    差       = %+ld ns\n",
               (long) ((p.tv_sec - t.tv_sec) * 1000000000L + (p.tv_nsec - t.tv_nsec)));
    }
    printf("\n");

    printf("== 3. 主动报出「刻度太粗」这件事 ==\n");
    {
        struct tms a, b;

        times(&a);
        { volatile int x = 0; for (int i = 0; i < 1000; i++) x += i; (void) x; }
        times(&b);
        printf("  跑一个 1000 次的小循环，tms_utime 的增量 = %ld tick = %.1f ms\n",
               (long) (b.tms_utime - a.tms_utime),
               (double) (b.tms_utime - a.tms_utime) * 1000.0 / (double) tck);
        printf("  → 常常是 0。不代表没耗 CPU，只代表不到 1 个 tick（%.1f ms）。\n",
               1000.0 / (double) tck);
        printf("  要测这种量级只能用 CLOCK_PROCESS_CPUTIME_ID（纳秒分辨率）。\n");
    }
    printf("\n");

    printf("== 4. 子进程的时间记在 tms_c* 里（RUSAGE_CHILDREN） ==\n");
    {
        pid_t pid = fork();

        if (pid == 0) {
            volatile unsigned long long s = 0;
            for (long i = 0; i < 40000000L; i++)
                s += (unsigned long long) i;
            _exit(0);
        } else if (pid > 0) {
            int st;
            struct rusage ru;

            wait(&st);
            snapshot("子进程烧完 CPU 后");
            if (getrusage(RUSAGE_CHILDREN, &ru) == 0) {
                printf("  getrusage(RUSAGE_CHILDREN): user=%ld.%06ld s  sys=%ld.%06ld s\n",
                       (long) ru.ru_utime.tv_sec, (long) ru.ru_utime.tv_usec,
                       (long) ru.ru_stime.tv_sec, (long) ru.ru_stime.tv_usec);
            }
            printf("  → 子进程的 CPU 时间只累加到**父进程**的 tms_cutime，"
                   "本进程自己的 tms_utime 不变\n");
        } else {
            printf("  fork 失败\n");
        }
    }

    printf("\n三个必须记住的点：\n");
    printf("  1) times() 与 clock() 单位**不同**：前者是 1/_SC_CLK_TCK，后者是 1/CLOCKS_PER_SEC；\n");
    printf("  2) 只要进程没在跑，CPU 时间就不涨 —— 测「服务响应延迟」要用墙上时间，\n");
    printf("     测「这段算法花了多少 CPU」才用 CPU 时间；\n");
    printf("  3) 所有 CPU 时间的口径都是**多核累加**：4 核跑满 1 秒，"
           "CLOCK_PROCESS_CPUTIME_ID 涨 4 秒。\n");
    return 0;
}
