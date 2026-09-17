/* c23_alarm_pipe_timeout.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。
      §23.3 的 Listing 23-2 是 `timed_read.c`，它演示「用定时器给阻塞系统调用
      加超时」。但它的 `Read timed out` 分支在**非交互环境**下极难复现：只要
      stdin 是「已关闭的管道/重定向的普通文件」，read() 会立刻返回 0（EOF），
      根本走不到超时那一步 —— 本仓库在 CE 上实测正是如此。

   本程序把「会真的阻塞下去的 fd」造出来：用一个**写端保持打开**的匿名管道，
   读端必然阻塞。于是同一条 5 步法能观察到两种截然不同的结果：

     对照 A（sa_flags = 0）          ⇒ SIGALRM 打断 read()，返回 -1 / errno = EINTR
     对照 B（sa_flags = SA_RESTART） ⇒ read() 被内核自动重启，永远不返回

   这样 §23.3 讲的「装上 SA_RESTART 与否」就不再是纸面差别。

   ⚠️ stdout 设为无缓冲（setvbuf _IONBF）：对照 B 会一直阻塞到被执行器杀掉。
      CE 的 stdout 是 **socket** ⇒ glibc 用 8KB 全缓冲 ⇒ 有缓冲的话前面已经
      打印的内容会跟着进程一起消失。这是本仓库实测环境的事实，不是程序行为。

   编译：gcc -O0 -Wall -Wextra -o c23_alarm_pipe c23_alarm_pipe_timeout.c
   ⚠️ 只依赖 libc，不需要 tlpi_hdr.h / get_num.c。

   ⚠️ 本文件**刻意不定义 `_POSIX_C_SOURCE`**，与 Listing 23-2（timed_read.c）
      的写法保持一致。原因实测踩过：一旦显式定义 `_POSIX_C_SOURCE 199309`，
      glibc 就**不再隐式打开 `_DEFAULT_SOURCE`**，`SA_RESTART` 随之变成
      `error: 'SA_RESTART' undeclared`。
      （原书 Makefile 之所以能编过 `ptmr_*.c` 那批带 `_POSIX_C_SOURCE` 的文件，
        是因为 IMPL_CFLAGS 里还有 `-D_DEFAULT_SOURCE`。）
*/
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t handlerCalls = 0;

static void
handler(int sig)
{
    (void) sig;                 /* 处理器什么都不做，只负责打断 read() */
    handlerCalls++;
}

int
main(void)
{
    int fds[2];
    char buf[64];
    ssize_t nr;
    int savedErrno;

    setvbuf(stdout, NULL, _IONBF, 0);   /* 见文件头：对照 B 会被杀，不能留缓冲 */

    if (pipe(fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    printf("pipe() 造出读端 fd=%d、写端 fd=%d —— 写端**故意不关**\n", fds[0], fds[1]);
    printf("所以 read(fd=%d, ...) 一定会阻塞。\n\n", fds[0]);

    /* ---------------- 对照 A：不加 SA_RESTART（Listing 23-2 的做法） -------- */

    printf("[对照 A] sa_flags = 0，alarm(1)\n");
    {
        struct sigaction sa;

        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = handler;
        if (sigaction(SIGALRM, &sa, NULL) == -1) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }

    handlerCalls = 0;
    alarm(1);                           /* 第 2 步：设超时上限 */
    nr = read(fds[0], buf, sizeof(buf)); /* 第 3 步：做阻塞系统调用 */
    savedErrno = errno;                 /* alarm() 会动 errno，先存 */
    alarm(0);                           /* 第 4 步：撤销定时器 */
    errno = savedErrno;

    printf("  read() 返回 %ld\n", (long) nr);
    if (nr == -1) {
        printf("  errno = %d (%s)\n", savedErrno,
               savedErrno == EINTR ? "EINTR" : "其它");
        printf("  => Listing 23-2 会走 \"Read timed out\" 分支\n");
    }
    printf("  handler 共被调用 %d 次\n\n", (int) handlerCalls);

    /* ---------------- 对照 B：加 SA_RESTART -------------------------------- */

    printf("[对照 B] sa_flags = SA_RESTART，alarm(1)\n");
    printf("  预期：read() 被自动重启，永远不返回 => 本行之后不会再有输出\n");
    {
        struct sigaction sa;

        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = handler;
        if (sigaction(SIGALRM, &sa, NULL) == -1) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }

    handlerCalls = 0;
    alarm(1);
    nr = read(fds[0], buf, sizeof(buf));
    savedErrno = errno;
    alarm(0);
    errno = savedErrno;

    printf("  read() 返回 %ld，errno = %d"
           "  <= 能打印出这一行说明本机行为与预期不同\n",
           (long) nr, savedErrno);
    printf("  handler 共被调用 %d 次\n", (int) handlerCalls);

    return 0;
}
