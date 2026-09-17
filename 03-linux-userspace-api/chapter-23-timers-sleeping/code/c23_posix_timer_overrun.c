/* c23_posix_timer_overrun.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。
      §23.6.6 讲「定时器 overrun」，原书的示例是靠手工 `Ctrl-Z`（SIGSTOP）挂起
      进程几秒再 `fg` 来制造 overrun。这种手法在无人值守环境（CI / CE / 自动化
      回归）里无法复现，所以本节在 CE 上从来观察不到 overrun > 0 的正例。

   本程序用**确定性的手法**制造 overrun —— 不需要任何人工干预：

     1. sigprocmask(SIG_BLOCK, {SIGRTMAX})  把通知信号阻塞住
        ⇒ 定时器到期产生的信号进不了处理器。
     2. timer_create(CLOCK_REALTIME, SIGEV_SIGNAL/SIGRTMAX)
     3. timer_settime(1 ms 后首次，之后每 1 ms)
     4. nanosleep(1 s)
        ⇒ 这一秒里定时器到期约 1000 次，但内核只保留**一个** pending 信号，
          多出来的到期**不会排队**（这正是 POSIX.1b 选择「计数」而不是
          「排队 realtime 信号」的原因，见原书 §23.6.6）。
     5. sigwaitinfo() 取那唯一一个信号，读 si_overrun / timer_getoverrun()

   预期：overrun ≈ 999（到期 ~1000 次，其中 1 次「兑现」成那个信号，其余算法 overrun）。

   编译：gcc -O0 -Wall -Wextra -o c23_overrun c23_posix_timer_overrun.c
   ⚠️ 只依赖 libc，不需要 tlpi_hdr.h / get_num.c。
*/
#define _POSIX_C_SOURCE 199309
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TIMER_SIG SIGRTMAX
#define NSEC_PER_MS 1000000L

int
main(void)
{
    timer_t tid;
    struct sigevent sev;
    struct itimerspec ts;
    struct timespec req, t0, t1;
    sigset_t blockSet;
    siginfo_t si;
    int got;

    /* ---- 时间基准用 CLOCK_MONOTONIC，避免 NTP 干扰（§23.5） ---- */

    if (clock_gettime(CLOCK_MONOTONIC, &t0) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    printf("=== POSIX 定时器 overrun 实测 ===\n");
    printf("通知信号 SIGRTMAX = %d\n", TIMER_SIG);

    /* ---- 第 1 步：先把通知信号阻塞住 ---- */

    sigemptyset(&blockSet);
    sigaddset(&blockSet, TIMER_SIG);
    if (sigprocmask(SIG_BLOCK, &blockSet, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
    printf("已 sigprocmask(SIG_BLOCK) 阻塞信号 %d —— 它进不了处理器，只能被排队\n",
           TIMER_SIG);

    /* ---- 第 2 步：创建定时器，通知方式 = 信号 ---- */

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TIMER_SIG;
    sev.sigev_value.sival_ptr = &tid;

    if (timer_create(CLOCK_REALTIME, &sev, &tid) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }
    printf("timer ID = %ld\n", (long) tid);

    /* ---- 第 3 步：武装 —— 1 ms 后首次，之后每 1 ms ---- */

    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = NSEC_PER_MS;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = NSEC_PER_MS;

    if (timer_settime(tid, 0, &ts, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    /* ---- 第 4 步：睡 1 秒，让到期次数远远超过「能送达的信号个数」 ---- */

    req.tv_sec = 1;
    req.tv_nsec = 0;
    if (nanosleep(&req, NULL) == -1) {
        perror("nanosleep");
        exit(EXIT_FAILURE);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    printf("睡了 %.6f 秒（墙钟 MONOTONIC 口径）—— 期间定时器到期约 1000 次\n",
           (double) (t1.tv_sec - t0.tv_sec)
               + (double) (t1.tv_nsec - t0.tv_nsec) / 1000000000.0);

    /* ---- 第 5 步：取那唯一一个信号，读 overrun ---- */

    got = sigwaitinfo(&blockSet, &si);
    if (got == -1) {
        perror("sigwaitinfo");
        exit(EXIT_FAILURE);
    }
    printf("sigwaitinfo() 取到信号 %d\n", got);

#ifdef __linux__
    printf("    si_overrun         = %d      (Linux 扩展字段，省一次系统调用)\n",
           si.si_overrun);
#endif
    printf("    timer_getoverrun() = %d      (SUSv3 规定的取法)\n",
           timer_getoverrun(tid));

    /* ---- 收尾：先解除武装，再看 overrun 是不是被「收到信号」这个动作重置了 ---- */

    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    if (timer_settime(tid, 0, &ts, NULL) == -1) {   /* it_value 全 0 = 解除武装 */
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    printf("解除武装后再读一次 timer_getoverrun() = %d\n", timer_getoverrun(tid));
    printf("读法：到期 N 次但只送出 1 个信号时，overrun = N - 1；\n");
    printf("      收到信号后 overrun 会被重置，所以第二次读到的是 0。\n");

    if (timer_delete(tid) == -1) {
        perror("timer_delete");
        exit(EXIT_FAILURE);
    }
    return 0;
}
