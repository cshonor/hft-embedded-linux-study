/* ex24_1_fork_count.c

   ⚠️ 本仓库自写 —— 原书习题 24-1 **没有官方解答文件**
      （官方只给了习题 24-2 的解：procexec/vfork_fd_test.c）。

   习题 24-1：一个程序连续执行
        fork(); fork(); fork();
   （假设都不失败）之后，一共产生多少个**新**进程？

   答案 2^3 - 1 = **7**（进程总数 8 = 2^3）。

   但「背公式」不算懂。本程序让结论**可数**：
     * 在**任何 fork 之前**建一条管道；
     * 3 次 fork 里，凡是「fork() 返回 0」的进程就标记 isNew = 1
       （注意：3 次 fork 里每个进程都要走完 3 轮，所以子进程还会继续 fork
         出孙进程 —— 这正是数量翻倍的原因）；
     * 每个「新进程」往管道里写一行自己的 pid/ppid，然后退出；
     * 原始进程关掉自己的写端，读干管道（EOF 要等所有写端关闭 ⇒ 等所有新进程
       退出），逐行回显并计数。

   编译（自包含，不需要 tlpi_hdr.h）：
     gcc -O0 -Wall -Wextra -o ex24_1_fork_count ex24_1_fork_count.c
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg)
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

int main(void)
{
    int pfd[2];
    int i;
    int isNew = 0;                 /* 原始进程保持 0；任何 fork 出来的进程置 1 */
    pid_t p;

    setvbuf(stdout, NULL, _IONBF, 0);

    if (pipe(pfd) == -1)           /* ⚠️ 必须在任何 fork 之前建好 */
        die("pipe");

    printf("原始进程 pid=%ld\n", (long) getpid());
    printf("--- 开始 fork(); fork(); fork(); ---\n");

    for (i = 0; i < 3; i++) {
        switch (p = fork()) {
        case -1:
            die("fork");
        case 0:
            isNew = 1;             /* 由 fork 诞生的进程都走这条分支 */
            break;
        default:
            break;                 /* 原始进程（以及所有"当爹"的进程）走这里 */
        }
    }

    if (isNew) {
        /* 新进程：报一行自己的身份，写完就退出（_exit 不刷 stdio 缓冲） */
        dprintf(pfd[1], "新进程 pid=%-6ld ppid=%-6ld\n",
                (long) getpid(), (long) getppid());
        close(pfd[1]);
        _exit(EXIT_SUCCESS);
    }

    /* ---------- 以下只有原始进程会走到 ---------- */
    close(pfd[1]);                 /* 关掉自己的写端，否则永远读不到 EOF */

    {
        char buf[4096];
        ssize_t n;
        int lines = 0;
        ssize_t k;

        while ((n = read(pfd[0], buf, sizeof buf)) > 0) {
            fwrite(buf, 1, (size_t) n, stdout);   /* 逐行回显（顺序不定）*/
            for (k = 0; k < n; k++)
                if (buf[k] == '\n')
                    lines++;
        }
        if (n == -1)
            die("read");

        printf("--- 新进程行数 = %d ---\n", lines);
        printf("--- 进程总数   = %d ---\n", lines + 1);
        printf("（2^3 = 8 个进程并存；新产生 8 - 1 = 7 个。行的顺序不定，\n"
               "  因为 fork 之后父子谁先跑由调度器决定 —— 见 §24.4）\n");
    }

    while (wait(NULL) > 0)         /* 收尸（只收得到直接子进程）*/
        ;
    _exit(EXIT_SUCCESS);
}
