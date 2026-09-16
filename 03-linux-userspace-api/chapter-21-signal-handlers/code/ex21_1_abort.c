/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2026.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* ex21_1_abort.c — 习题 21-1：实现 abort()

   SUSv3 要求的 abort() 语义（TLPI 21.2.2 / P433）：
   1. 无论 SIGABRT 被阻塞还是忽略，abort() 都必须终止进程（override 阻塞/忽略）
   2. 若进程用 handler 捕获 SIGABRT，先让 handler 跑；
      handler 若返回，则 abort() 把处置复位为 SIG_DFL 再 raise 一次 → 必死
   3. 终止前 flush 并关闭 stdio 流

   本程序实现 my_abort()，并用三个场景验证：
     ⓪ 默认处置：直接终止
     ① SIGABRT 被阻塞：解除阻塞语义 → 仍终止
     ② handler 捕获并返回：复位 SIG_DFL → 第二次 raise 终止
   实测环境：macOS 26.6.2 (arm64)，clang 23.1.0
*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* —— 习题 21-1 的核心：my_abort() —— */
static void
my_abort(void)
{
    sigset_t mask;
    struct sigaction sa;

    /* SUSv3：abort() 必须无视阻塞 —— 先解除 SIGABRT 的阻塞 */
    sigemptyset(&mask);
    sigaddset(&mask, SIGABRT);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        _exit(EXIT_FAILURE);
    }

    /* 查当前处置：若装了 handler，让它跑（handler 可能 longjmp 不回来） */
    if (sigaction(SIGABRT, NULL, &sa) == -1) {
        perror("sigaction");
        _exit(EXIT_FAILURE);
    }
    if (sa.sa_handler != SIG_DFL && sa.sa_handler != SIG_IGN)
        raise(SIGABRT);                 /* handler 返回 = 没终止进程 */

    /* 走到这里说明 handler 返回（或原处置是 SIG_IGN）：
       复位 SIG_DFL，flush stdio，再 raise 一次 —— 这次必死 */
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGABRT, &sa, NULL) == -1) {
        perror("sigaction reset");
        _exit(EXIT_FAILURE);
    }
    fflush(stdout);
    raise(SIGABRT);

    /* 若信号仍被忽略（理论上不可能，SIG_DFL 已复位）：兜底 */
    _exit(EXIT_FAILURE);
}

/* —— 场景 ② 的 handler：捕获后直接返回 —— */
static void
handler(int sig)
{
    printf("    handler: 捕获 SIGABRT(%d) 后返回 → my_abort 应回位 SIG_DFL 再来一次\n", sig);
}

int
main(int argc, char *argv[])
{
    printf("PID=%ld  用法：无参=⓪默认；b=①阻塞；h=②handler 返回\n",
            (long) getpid());

    if (argc > 1 && argv[1][0] == 'b') {
        /* ① 阻塞 SIGABRT 再 abort —— SUSv3：必须仍终止 */
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGABRT);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        printf("[①] SIGABRT 已阻塞，调 my_abort() —— 应仍然终止\n");
        printf("[①] 这行**能**打出来：abort() 终止前必须 flush stdio（SUSv3 要求）\n");
        my_abort();

    } else if (argc > 1 && argv[1][0] == 'h') {
        /* ② handler 捕获并返回 —— 复位 SIG_DFL 后第二次 raise 终止 */
        struct sigaction sa;
        sa.sa_handler = handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGABRT, &sa, NULL);
        printf("[②] 装 handler，调 my_abort()\n");
        my_abort();

    } else {
        /* ⓪ 默认处置直接终止 */
        printf("[⓪] 默认处置，调 my_abort() —— 直接 SIGABRT 终止\n");
        my_abort();
    }

    printf("不应到达这里\n");
    exit(EXIT_SUCCESS);
}
