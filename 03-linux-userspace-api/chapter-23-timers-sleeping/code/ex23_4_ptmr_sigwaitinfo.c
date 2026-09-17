/* ex23_4_ptmr_sigwaitinfo.c —— TLPI 习题 23-4 的实现
 *
 * 题面（原书 §23.9，逐字）：
 *     "Modify the program in Listing 23-5 (ptmr_sigev_signal.c) to use
 *      sigwaitinfo() instead of a signal handler."
 *
 * 与 Listing 23-5（`code/ptmr_sigev_signal.c`）的差别只有一处，但含义很大：
 *
 *   Listing 23-5：sigaction(TIMER_SIG, SA_SIGINFO, handler) + `for (;;) pause();`
 *                 ⇒ 通知**异步**送到信号处理器里
 *   本程序     ：sigprocmask(SIG_BLOCK, {TIMER_SIG}) + `sigwaitinfo(&set, &si)`
 *                 ⇒ 通知**同步**取出来，没有信号处理器、没有异步信号上下文
 *
 * ⭐ 为什么要先 sigprocmask() 阻塞？
 *     sigwaitinfo() 只有在信号**处于阻塞状态**时才是可靠的：如果信号没被阻塞，
 *     它可能在 sigwaitinfo() 被调用**之前**就按默认处置送达（SIGRTMAX 的默认
 *     动作是终止进程）；更糟的是，即便装了处理器，通知也可能被处理器抢先领走，
 *     让 sigwaitinfo() 永远等下去。先阻塞 → 信号只会挂在 pending 集合里 →
 *     由 sigwaitinfo() 独占领取。这是 §22.10 的核心结论。
 *
 * ⭐ 由此带来的两个好处（正是 sigwaitinfo 存在的理由）：
 *     1. **没有异步信号上下文** ⇒ 可以直接调用 printf / malloc 等非异步信号
 *        安全函数，不必像 Listing 23-5 那样在注释里写 "UNSAFE"。
 *     2. **可以拿到 siginfo 的完整信息**，包括定时器专用的 `si_overrun`
 *        （Linux 扩展）——它和 timer_getoverrun() 给的是同一个数。
 *
 * ⚠️ 本程序相对题面的**唯一扩展**（明确声明，不是原书内容）：
 *     Listing 23-5 的 `for (;;) pause();` 没有退出路径；为了让它在无人值守的
 *     环境里也能跑完并留下完整输出，本程序加了一个 `-n <N>` 选项：收到 N 次
 *     通知后正常退出（`-n 0` / 不写 = 与原书一样无限等待）。
 *
 * 用法：./ex23_4_ptmr_sigwaitinfo [-n max-notify] secs[/nsecs][:int-secs[/int-nsecs]]...
 *
 * 编译：gcc -O0 -Wall -Wextra -o ex23_4_ptmr_sigwaitinfo \
 *           ex23_4_ptmr_sigwaitinfo.c curr_time.c itimerspec_from_str.c
 *
 * 源码坐标（Linux v6.6）：
 *   kernel/signal.c            sigwaitinfo() 走 do_sigtimedwait()，直接操作
 *                              pending 集合，**不经过信号投递流程**
 *   kernel/time/posix-timers.c common_timer_get() → si_overrun 的来源
 *                              （即 timer_getoverrun() 的字段级出处）
 */
#define _POSIX_C_SOURCE 199309
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curr_time.h"                  /* Declares currTime() */
#include "itimerspec_from_str.h"        /* Declares itimerspecFromStr() */
#include "tlpi_hdr.h"

#define TIMER_SIG SIGRTMAX              /* 与 Listing 23-5 保持一致 */

int
main(int argc, char *argv[])
{
    struct itimerspec ts;
    struct sigevent   sev;
    timer_t *tidlist;
    sigset_t blockSet;
    int j, opt, maxNotify = 0, nNotify = 0;

    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            maxNotify = atoi(optarg);
            break;
        default:
            usageErr("%s [-n max-notify] "
                     "secs[/nsecs][:int-secs[/int-nsecs]]...\n", argv[0]);
        }
    }

    if (optind >= argc)
        usageErr("%s [-n max-notify] "
                 "secs[/nsecs][:int-secs[/int-nsecs]]...\n", argv[0]);

    tidlist = calloc(argc - optind, sizeof(timer_t));
    if (tidlist == NULL)
        errExit("calloc");

    /* ★ 关键差异 1：不做 sigaction()，改成把 TIMER_SIG 阻塞掉。
       （先阻塞、后建定时器 —— 顺序反了就会丢掉最早几次通知。） */

    sigemptyset(&blockSet);
    sigaddset(&blockSet, TIMER_SIG);
    if (sigprocmask(SIG_BLOCK, &blockSet, NULL) == -1)
        errExit("sigprocmask");

    /* 创建并启动每个命令行参数对应的一个定时器 —— 与 Listing 23-5 完全相同 */

    sev.sigev_notify = SIGEV_SIGNAL;    /* Notify via signal */
    sev.sigev_signo = TIMER_SIG;        /* Notify using this signal */

    for (j = 0; j < argc - optind; j++) {
        itimerspecFromStr(argv[j + optind], &ts);

        sev.sigev_value.sival_ptr = &tidlist[j];
                /* Allows the waiter to get ID of this timer */

        if (timer_create(CLOCK_REALTIME, &sev, &tidlist[j]) == -1)
            errExit("timer_create");
        printf("Timer ID: %ld (%s)\n", (long) tidlist[j], argv[j + optind]);

        if (timer_settime(tidlist[j], 0, &ts, NULL) == -1)
            errExit("timer_settime");
    }

    /* ★ 关键差异 2：同步等待，而不是 pause() + 处理器 */

    for (;;) {
        siginfo_t si;
        int sig;
        timer_t *tidptr;

        sig = sigwaitinfo(&blockSet, &si);
        if (sig == -1) {
            if (errno == EINTR) {
                printf("    （sigwaitinfo 被无关信号打断，重试）\n");
                continue;
            }
            errExit("sigwaitinfo");
        }

        tidptr = si.si_value.sival_ptr;

        /* 这里 printf 是**安全的** —— 同步上下文，不是异步信号处理器 */
        printf("[%s] sigwaitinfo() got signal %d\n", currTime("%T"), sig);
        printf("    *sival_ptr         = %ld\n", (long) *tidptr);
        printf("    si_overrun         = %d\n", si.si_overrun);
        printf("    timer_getoverrun() = %d\n", timer_getoverrun(*tidptr));

        nNotify++;
        if (maxNotify > 0 && nNotify >= maxNotify) {
            printf("已经收到 %d 次通知，按 -n 限定退出（注意：未阻塞前的事件数不保证）\n",
                   nNotify);
            break;
        }
    }

    for (j = 0; j < argc - optind; j++)
        timer_delete(tidlist[j]);
    free(tidlist);

    printf("\n=== ex23_4 done ===\n");
    exit(EXIT_SUCCESS);
}
