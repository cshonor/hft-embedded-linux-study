/* TLPI 第 2 章 §2.13 —— 进程组与作业控制：kill(-pgid) 的广播语义
 *
 * 编译：gcc -O0 -Wall -Wextra c2_13_pgroup.c -o c2_13
 * 运行：./c2_13
 *
 * 本节要钉死的事实：
 *   ① 进程组是「一组进程的集合」，pgid == 组长的 pid。fork 出的子进程默认同组。
 *   ② setpgid(0, 0) 让调用者自成一组（自己当组长）。
 *   ③ kill() 的 pid 参数为**负数**时表示「发给 |pid| 这个进程组的所有成员」。
 *   ④ 这正是 shell 的 Ctrl+C / Ctrl+Z 能一次性作用到整条管道的原因 ——
 *      终端驱动把信号发给「前台进程组」，不是发给某个进程。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

static void report_status(const char *tag, int status)
{
    if (WIFEXITED(status))
        printf("  %s: 正常退出 code=%d\n", tag, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("  %s: 被信号 %d (%s) 杀死\n", tag, WTERMSIG(status),
               strsignal(WTERMSIG(status)));
    else
        printf("  %s: status=0x%x\n", tag, status);
}

int main(void)
{
    printf("=== ① 我自己的进程组 ===\n");
    printf("  getpid()      = %d\n", (int)getpid());
    printf("  getpgrp()     = %d\n", (int)getpgrp());
    printf("  getpgid(0)    = %d\n", (int)getpgid(0));
    printf("  getppid()     = %d\n", (int)getppid());
    printf("  -> pid == pgid，说明本进程是进程组的**组长（leader）**\n");

    printf("\n=== ② fork 出的子进程默认在同一个组 ===\n");
    int ready[2];
    if (pipe(ready) < 0) { perror("pipe"); return 1; }

    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* ---------------- 子进程 ---------------- */
        close(ready[0]);
        printf("  [child] 刚 fork 出来：pid=%d  pgid=%d  （pgid 就是父的 pid）\n",
               (int)getpid(), (int)getpgrp());

        if (setpgid(0, 0) < 0)                  /* 0,0 = 以自己为组长新建一组 */
            printf("  [child] setpgid(0,0) 失败 errno=%d (%s)\n", errno, strerror(errno));
        else
            printf("  [child] setpgid(0,0) 后：pid=%d  pgid=%d  （自成一组了）\n",
                   (int)getpid(), (int)getpgrp());
        fflush(NULL);

        char b = 'x';
        if (write(ready[1], &b, 1) < 0) { /* 告诉父进程「我准备好了」 */ }
        close(ready[1]);

        pause();                                /* 挂着等信号 */
        fflush(NULL);
        _exit(0);                               /* SIGTERM 默认会直接终止，走不到这 */
    }

    /* ---------------- 父进程 ---------------- */
    close(ready[1]);
    char b;
    if (read(ready[0], &b, 1) < 0) { /* 等子进程就绪 */ }
    close(ready[0]);

    pid_t cpgid = getpgid(pid);
    printf("  父进程验证：子进程 %d 现在属于进程组 %d\n", (int)pid, (int)cpgid);
    printf("  父进程自己在进程组 %d\n", (int)getpgrp());

    printf("\n=== ③ kill(-pgid, sig)：给整组广播 ===\n");
    if (cpgid != getpgrp()) {
        printf("  安全前提成立（子进程的组 != 父的组），广播不会伤到自己\n");
        printf("  kill(%d, SIGTERM)  <- 负号 = 发给整个进程组\n", -(int)cpgid);
        if (kill(-cpgid, SIGTERM) < 0) perror("kill(-pgid)");
    } else {
        printf("  ⚠️ 子进程还在父进程的组里 —— 广播会连本进程一起杀\n");
        printf("     所以这里退化成给单个进程发：kill(%d, SIGTERM)\n", (int)pid);
        if (kill(pid, SIGTERM) < 0) perror("kill(pid)");
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) perror("waitpid");
    else report_status("child", st);

    printf("\n=== ④ 为什么 Ctrl+C 能杀掉整条管道 ===\n");
    printf("  shell 把 `ls | grep | wc` 里的三个进程放进**同一个进程组**，\n");
    printf("  并把它设为终端的「前台进程组」。你按 Ctrl+C 时：\n");
    printf("    终端驱动 → 给前台进程组发 SIGINT（等价 kill(-pgid, SIGINT)）\n");
    printf("    → 三个进程一起收到，一起结束\n");
    printf("  Ctrl+Z 同理，发的是 SIGTSTP；shell 收到状态变化后打印 [1]+ Stopped，\n");
    printf("  fg/bg 时再调 tcsetpgrp() 切换前台组 + 发 SIGCONT。\n");
    printf("  → 作业控制不是 shell 的魔法，是「进程组 + 会话」两套内核机制的编排。\n");

    printf("\n=== ⑤ 与进程组相关的常用调用 ===\n");
    printf("  %-26s %s\n", "getpgrp()", "取本进程的 pgid（= getpgid(0)）");
    printf("  %-26s %s\n", "getpgid(pid)", "取指定进程的 pgid");
    printf("  %-26s %s\n", "setpgid(pid, pgid)", "把 pid 放进 pgid 组；setpgid(0,0) = 自成一组");
    printf("  %-26s %s\n", "kill(-pgid, sig)", "给整个组广播信号");
    printf("  %-26s %s\n", "waitpid(-pgid, ...)", "等整个组里的任意子进程（pid 也可以给负数）");
    printf("  %-26s %s\n", "tcsetpgrp(fd, pgid)", "把某个进程组设为终端的前台组（作业控制核心）");
    return 0;
}
