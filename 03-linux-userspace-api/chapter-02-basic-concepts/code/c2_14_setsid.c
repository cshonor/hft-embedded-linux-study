/* TLPI 第 2 章 §2.14 —— 会话、控制终端：daemon 化的三步
 *
 * 编译：gcc -O0 -Wall -Wextra c2_14_setsid.c -o c2_14
 * 运行：./c2_14
 *
 * 本节要钉死的事实：
 *   ① 层级是「会话 > 进程组 > 进程」：sid、pgid、pid 三个数字定位一个进程。
 *   ② 会话首领（session leader）拥有控制终端（controlling terminal）。
 *   ③ setsid() 新建会话 + 新进程组并丢掉控制终端；但**进程组组长调它会 EPERM**。
 *   ④ 所以 daemon 的标准姿势是「先 fork 让子进程不再是组长，再 setsid」。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(void)
{
    printf("=== ① 三组 ID：这个进程站在哪一层 ===\n");
    printf("  pid        = %d\n", (int)getpid());
    printf("  ppid       = %d\n", (int)getppid());
    printf("  pgid       = %d   （进程组）\n", (int)getpgrp());
    printf("  sid        = %d   （会话）\n", (int)getsid(0));
    printf("  pid==pgid  ? %s  → 本进程是进程组组长\n", getpid() == getpgrp() ? "YES" : "no");
    printf("  pid==sid   ? %s  → 本进程是会话首领\n", getpid() == getsid(0) ? "YES" : "no");

    printf("\n=== ② 控制终端：有还是没有 ===\n");
    printf("  ttyname(STDIN_FILENO) = %s\n",
           ttyname(STDIN_FILENO) ? ttyname(STDIN_FILENO) : "(null)  → 没有控制终端");
    char ct[64];
    ctermid(ct);
    printf("  ctermid()             = \"%s\"\n", ct);
    printf("    ↑ 注意：这只是「标准答案路径」，不代表它真的存在。实测：\n");
    printf("    access(\"/dev/tty\")   = %d", access("/dev/tty", F_OK));
    if (access("/dev/tty", F_OK) < 0) printf("  （%s）", strerror(errno));
    printf("\n");
    errno = 0;
    printf("    isatty(0) = %d   isatty(1) = %d   （0 = 不是终端）\n",
           isatty(0), isatty(1));
    printf("  -> 本环境下 stdin/stdout 是管道/套接字，所以没有终端语义。\n");
    printf("     「是不是终端」用 isatty() 判断，不要拿 ctermid() 当证据。\n");

    printf("\n=== ③ 进程组组长调 setsid() 会 EPERM ===\n");
    fflush(NULL);
    errno = 0;
    pid_t r = setsid();
    printf("  本进程（pid==pgid，是组长）调 setsid() → 返回 %d", (int)r);
    if (r < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    printf("  -> EPERM(%d)：内核拒绝「组长新建会话」，否则组和会话的对应关系会乱。\n", EPERM);
    errno = 0;

    printf("\n=== ④ 正确姿势：先 fork，让子进程当「非组长」再 setsid ===\n");
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* ---- 子进程：此刻不是组长（它的 pid 是新号，但 pgid 还是父的） ---- */
        printf("  [child] fork 后：pid=%d  pgid=%d  sid=%d\n",
               (int)getpid(), (int)getpgrp(), (int)getsid(0));

        errno = 0;
        pid_t ns = setsid();
        if (ns < 0) {
            printf("  [child] setsid() 失败 errno=%d (%s)\n", errno, strerror(errno));
        } else {
            printf("  [child] setsid() = %d  → 新会话成立了\n", (int)ns);
        }
        printf("  [child] setsid 之后：pid=%d  pgid=%d  sid=%d\n",
               (int)getpid(), (int)getpgrp(), (int)getsid(0));
        printf("  [child] 三者相等 → 我既是会话首领又是组长，且已脱离原控制终端\n");

        /* 再调一次会怎样？此刻已经是组长了 */
        errno = 0;
        pid_t again = setsid();
        printf("  [child] 再调一次 setsid() → %d", (int)again);
        if (again < 0) printf("  errno=%d (%s)", errno, strerror(errno));
        printf("\n  [child] -> 连会话首领自己也改不了自己所在的会话，语义上说得通。\n");
        errno = 0;

        printf("  [child] ttyname(0) = %s\n",
               ttyname(0) ? ttyname(0) : "(null)  → 确实没有控制终端了");
        fflush(NULL);
        _exit(0);
    }

    int st;
    if (waitpid(pid, &st, 0) < 0) perror("waitpid");
    else printf("  父进程：子进程 %d 退出码 %d\n", (int)pid, WEXITSTATUS(st));

    printf("\n=== ⑤ daemon 化标准流程（为什么是这几步）===\n");
    printf("  ① fork() 后父进程 _exit(0)\n");
    printf("     目的：让子进程变成孤儿，被 init 收养，且**不再是进程组组长**\n");
    printf("  ② setsid()\n");
    printf("     目的：新会话 + 新进程组 + 丢掉控制终端 → Ctrl+C/SIGHUP 再也找不到它\n");
    printf("  ③ （可选）再 fork 一次\n");
    printf("     目的：确保永远不是会话首领，也就永远拿不到控制终端\n");
    printf("  ④ chdir(\"/\")\n");
    printf("     目的：不占住某个挂载点，免得 umount/文件系统维护被卡\n");
    printf("  ⑤ 把 0/1/2 重定向到 /dev/null\n");
    printf("     目的：免得某个已关闭的终端 fd 被误用，或输出把磁盘写爆\n");
    printf("  ⑥ 关掉继承来的、不需要的 fd（多进程环境下应该用 O_CLOEXEC 预防）\n");

    printf("\n=== ⑥ 本环境的实测边界 ===\n");
    errno = 0;
    int tty_ok = access("/dev/tty", F_OK);
    printf("  · access(\"/dev/tty\", F_OK) = %d", tty_ok);
    if (tty_ok < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    errno = 0;
    int ptmx_ok = access("/dev/ptmx", F_OK);
    printf("  · access(\"/dev/ptmx\", F_OK) = %d", ptmx_ok);
    if (ptmx_ok < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    errno = 0;
    printf("  · 所以「终端」这类实验只能验到语义层：\n");
    printf("    pid/pgid/sid 三个数字的关系、setsid 的成败与 EPERM 条件，都是真的；\n");
    printf("    但「Ctrl+C 打到哪个组」需要真终端，容器里无法复现。\n");
    return 0;
}
