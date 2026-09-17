/* ex24_5_fork_sig_sync2.c

   ⚠️ 本仓库自写 —— 原书习题 24-5 **没有官方解答文件**
      （官方只给了习题 24-2 的解：procexec/vfork_fd_test.c）。

   习题 24-5：如果**子进程也要等父进程**完成某个动作，Listing 24-6
              （fork_sig_sync.c）需要怎么改？

   答：一个信号不够了，要**两个**信号做双向握手：
         子 → 父 用 SIGUSR1（"我干完了"）
         父 → 子 用 SIGUSR2（"我干完了"）
       并且**两个信号都要在 fork() 之前屏蔽**。原因是 Listing 24-6 里
       已经写明的那条：如果先 fork 再屏蔽，那么"信号已经发出"与"我开始屏蔽"
       之间存在窗口，对方那一枪可能在你 sigsuspend 之前就打完了 ——
       这正是要避免的那个竞态。

   本程序的输出**顺序是确定的**（靠信号保证），这跟 Listing 24-5 的
   fork_whos_on_first 形成对照：那里没有同步，先后完全看调度器。

   编译（需要 curr_time.c 提供 currTime()，见 code/README.md）：
     gcc -O0 -Wall -Wextra -o ex24_5_fork_sig_sync2 \
         ex24_5_fork_sig_sync2.c curr_time.c
*/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "curr_time.h"          /* currTime() */

#define CHILD_READY SIGUSR1     /* 子 → 父 */
#define PARENT_DONE SIGUSR2     /* 父 → 子 */

static void die(const char *msg)
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

static void handler(int sig)    /* 什么都不做，只为让 sigsuspend 醒过来 */
{
    (void) sig;
}

static void waitFor(sigset_t *emptyMask)
{
    if (sigsuspend(emptyMask) == -1 && errno != EINTR)
        die("sigsuspend");
}

int main(void)
{
    pid_t childPid;
    sigset_t blockMask, emptyMask;
    struct sigaction sa;

    setvbuf(stdout, NULL, _IONBF, 0);

    /* ---------- 屏蔽**两个**信号：必须在 fork() 之前 ---------- */
    sigemptyset(&blockMask);
    sigaddset(&blockMask, CHILD_READY);
    sigaddset(&blockMask, PARENT_DONE);
    if (sigprocmask(SIG_BLOCK, &blockMask, NULL) == -1)
        die("sigprocmask(SIG_BLOCK)");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;
    if (sigaction(CHILD_READY, &sa, NULL) == -1)
        die("sigaction(SIGUSR1)");
    if (sigaction(PARENT_DONE, &sa, NULL) == -1)
        die("sigaction(SIGUSR2)");

    switch (childPid = fork()) {
    case -1:
        die("fork");

    case 0:   /* ---------- 子进程 ---------- */
        printf("[%s %ld] Child  started - working\n",
                currTime("%T"), (long) getpid());
        sleep(2);                                   /* 模拟干活 */

        printf("[%s %ld] Child  -> SIGUSR1 to parent\n",
                currTime("%T"), (long) getpid());
        if (kill(getppid(), CHILD_READY) == -1)
            die("kill(SIGUSR1)");

        /* 反过来等父进程（习题 24-5 新增的部分）*/
        printf("[%s %ld] Child  waiting for parent\n",
                currTime("%T"), (long) getpid());
        sigemptyset(&emptyMask);
        waitFor(&emptyMask);

        printf("[%s %ld] Child  got SIGUSR2 -> done\n",
                currTime("%T"), (long) getpid());
        _exit(EXIT_SUCCESS);

    default:  /* ---------- 父进程 ---------- */
        printf("[%s %ld] Parent waiting for child\n",
                currTime("%T"), (long) getpid());
        sigemptyset(&emptyMask);
        waitFor(&emptyMask);                        /* 等 SIGUSR1 */

        printf("[%s %ld] Parent got SIGUSR1 - working\n",
                currTime("%T"), (long) getpid());
        sleep(1);                                   /* 模拟干活 */

        printf("[%s %ld] Parent -> SIGUSR2 to child\n",
                currTime("%T"), (long) getpid());
        if (kill(childPid, PARENT_DONE) == -1)
            die("kill(SIGUSR2)");

        if (wait(NULL) == -1)
            die("wait");
        printf("[%s %ld] Parent done\n", currTime("%T"), (long) getpid());
        exit(EXIT_SUCCESS);
    }
}
