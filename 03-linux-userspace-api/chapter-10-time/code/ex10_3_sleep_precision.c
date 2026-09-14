/* TLPI 第 10 章 §10.6/§10.8 配套自编练习（非原书习题）
 * 睡眠到底准不准：nanosleep 的亚 jiffy 能力 / sleep(1) 的误差 / 被信号打断怎么补
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex10_3_sleep_precision ex10_3_sleep_precision.c
 * 运行: ./ex10_3_sleep_precision                (无需参数；跑几秒)
 *
 * 三个要看的点：
 *   1. 请求 1 us、10 us 这种远小于 1 个 jiffy（本机 10 ms）的睡眠，
 *      **确实能睡着** —— 这是 hrtimer 而不是 jiffies 在支撑；
 *   2. 但「睡得短」不等于「睡得准」：请求越小，固定开销占比越大，
 *      实测值普遍大于请求值（只能晚了醒，不能早了醒）；
 *   3. nanosleep 被信号打断会返回 -1/EINTR 并报出**剩余时长**，
 *      补睡要用这个剩余量，不能掐头重睡。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static void probe_nanosleep(long ns)
{
    struct timespec req = { ns / 1000000000L, ns % 1000000000L };
    double t0, d;
    int rc;

    t0 = now();
    rc = nanosleep(&req, NULL);
    d = now() - t0;
    printf("  请求 %9ld ns (%8.3f us) -> 实际 %10.3f us  超出 %+9.3f us   rc=%d\n",
           ns, (double) ns / 1000.0, d * 1e6, d * 1e6 - (double) ns / 1000.0, rc);
}

static volatile sig_atomic_t got_alarm = 0;

static void on_alarm(int sig)
{
    (void) sig;
    got_alarm = 1;
}

int main(void)
{
    long tck = sysconf(_SC_CLK_TCK);
    struct sigaction sa;
    struct itimerval it;
    struct timespec req, rem;
    double t0, d;
    int rc;

    printf("== 0. 参照刻度 ==\n");
    printf("sysconf(_SC_CLK_TCK) = %ld  ->  1 个 jiffy = %.1f ms\n",
           tck, 1000.0 / (double) tck);
    printf("（下面请求 1 us 只有 1 个 jiffy 的 %.4f%%）\n\n",
           100.0 * 0.001 / (1000.0 / (double) tck));

    printf("== 1. nanosleep 的亚 jiffy 能力 ==\n");
    probe_nanosleep(1000L);            /* 1 us   */
    probe_nanosleep(10000L);           /* 10 us  */
    probe_nanosleep(100000L);          /* 100 us */
    probe_nanosleep(1000000L);         /* 1 ms   */
    probe_nanosleep(10000000L);        /* 10 ms  = 1 个 jiffy */
    probe_nanosleep(100000000L);       /* 100 ms */
    printf("  → 1 us 也叫得醒，说明走的不是 jiffies；但 1 us 的实测普遍在\n");
    printf("    几十 us 量级 —— 那部分是与调度器往返的固定开销，不是精度问题。\n\n");

    printf("== 2. sleep(1) 的误差 ==\n");
    t0 = now();
    rc = sleep(1);
    d = now() - t0;
    printf("  sleep(1) 实际 %.4f s，误差 %+.4f s，返回值 rc=%d（未被打断应为 0）\n",
           d, d - 1.0, rc);
    printf("  → sleep 的参数是秒，返回的是「剩余未睡完的秒数」，被打断时 != 0。\n\n");

    printf("== 3. 被信号打断：返回 EINTR 并给出剩余量 ==\n");
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                   /* 关键：不设 SA_RESTART，才看得到 EINTR */
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        printf("  sigaction 失败: %s\n", strerror(errno));
        return 1;
    }

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 50000;       /* 50 ms 后发一次 SIGALRM */
    setitimer(ITIMER_REAL, &it, NULL);

    req.tv_sec = 0;
    req.tv_nsec = 500 * 1000 * 1000;   /* 想睡 500 ms */
    rem.tv_sec = rem.tv_nsec = 0;

    errno = 0;
    t0 = now();
    rc = nanosleep(&req, &rem);
    d = now() - t0;
    printf("  nanosleep(500 ms) -> rc=%d  errno=%d (%s)\n", rc, errno, strerror(errno));
    printf("    实际睡了   %.1f ms\n", d * 1000);
    printf("    remaining   %.1f ms   <- 这就是「还差多少没睡」\n",
           (double) rem.tv_sec * 1000 + (double) rem.tv_nsec / 1e6);
    printf("    信号到了吗? %s\n", got_alarm ? "到了" : "没到");
    printf("\n  正确补睡写法（用 remaining 原地重启，而不是重新睡 500 ms）：\n");
    printf("      while (nanosleep(&req, &req) == -1 && errno == EINTR)\n");
    printf("          ;   /* 用返回的剩余量覆盖 req */\n");

    printf("\n== 4. 现场演示「原地重启」 ==\n");
    it.it_value.tv_usec = 30000;       /* 30 ms */
    setitimer(ITIMER_REAL, &it, NULL);
    got_alarm = 0;
    req.tv_sec = 0;
    req.tv_nsec = 200 * 1000 * 1000;   /* 目标：睡满 200 ms */
    t0 = now();
    while (nanosleep(&req, &req) == -1 && errno == EINTR)
        ;                              /* 被 SIGALRM 打断就带着剩余量再来 */
    d = now() - t0;
    printf("  目标 200 ms，实际 %.3f ms，期间被打断 %s\n",
           d * 1000, got_alarm ? "过（已自动补睡）" : "没被打断");
    printf("  → 补睡的正确终点是「累计睡满」，不是「睡过一次就完事」。\n");
    return 0;
}
