/* c24_sigmask_inherit.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。
      TLPI Ch24 正文只讲了「父子各拿到 stack/data/heap 的副本」（§24.2，
   Listing 24-1）与「共享同一个 open file description」（§24.2.1，Listing 24-2），
   **并没有**给出 fork() 的完整「继承 / 不继承」清单，也没有演示下面这两条。

   本程序用一对可确定性观察的探针把它们钉死：

     ① 信号掩码（signal mask）**继承**
        父进程里 SIGUSR1 处于屏蔽态 ⇒ 子进程里查询掩码，SIGUSR1 仍是屏蔽的。

     ② pending 信号**不继承**
        父进程先屏蔽 SIGUSR1，再 raise(SIGUSR1) ⇒ 这个信号变成「已产生但未递送」
        （pending）。此时 fork()：父进程的 sigpending() 里有 SIGUSR1，
        子进程的 sigpending() 是**空集**。

   依据：
     * POSIX fork(): "the set of signals pending for the child process shall be
       initialized to the empty set"
     * man 2 fork 的 NOTES 一节（Linux man-pages）列出 fork 后**不**继承的项；
       信号掩码在 "inherits" 那一侧，pending 集在 "does not inherit" 那一侧。

   ⚠️ 为什么原书 Ch24 没讲：TLPI 把 fork 的进程属性继承放在别处 —— 记录锁在
      §29（`man 7 fcntl` 亦如此）、线程的 pending 集在 Ch33。Ch24 只谈
      「内存副本 + 打开文件共享 + 调度顺序」这三件事。本节把这条常见考点补齐。

   编译（无需 tlpi_hdr.h，自包含）：
     gcc -O0 -Wall -Wextra -o c24_sigmask_inherit c24_sigmask_inherit.c
*/

#include <errno.h>
#include <signal.h>
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
    sigset_t blockSet, curSet, pendingSet;
    pid_t childPid;

    /* 关掉 stdout 缓冲：父子两段输出不会互相污染（CE 的 stdout 是 socket，
       glibc 会按 8KB 全缓冲 —— 不关的话父子各持一份副本缓冲，容易看不出真相）*/
    setvbuf(stdout, NULL, _IONBF, 0);

    /* ---------- 父进程：屏蔽 SIGUSR1，并让它变成 pending ---------- */
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &blockSet, NULL) == -1)
        die("sigprocmask(SIG_BLOCK)");

    if (raise(SIGUSR1) != 0)          /* 自己给自己发；因为被屏蔽 ⇒ 停在 pending */
        die("raise(SIGUSR1)");

    if (sigprocmask(SIG_BLOCK, NULL, &curSet) == -1)   /* 查询当前掩码 */
        die("sigprocmask(查询掩码)");
    if (sigpending(&pendingSet) == -1)
        die("sigpending");

    printf("父进程 pid=%ld ppid=%ld\n", (long) getpid(), (long) getppid());
    printf("  掩码 中 SIGUSR1 = %d   （1 = 屏蔽）\n", sigismember(&curSet, SIGUSR1));
    printf("  pending 中 SIGUSR1 = %d   （1 = 有未递送信号）\n",
            sigismember(&pendingSet, SIGUSR1));

    /* ---------- fork：掩码与 pending 的继承差异就在这一刻分开 ---------- */
    switch (childPid = fork()) {
    case -1:
        die("fork");

    case 0:   /* 子进程：什么都不改，只把继承到的东西读出来 */
        if (sigprocmask(SIG_BLOCK, NULL, &curSet) == -1)
            die("child sigprocmask(查询掩码)");
        if (sigpending(&pendingSet) == -1)
            die("child sigpending");

        printf("子进程 pid=%ld ppid=%ld\n", (long) getpid(), (long) getppid());
        printf("  掩码 中 SIGUSR1 = %d   （继承来的）\n", sigismember(&curSet, SIGUSR1));
        printf("  pending 中 SIGUSR1 = %d   （不继承）\n",
                sigismember(&pendingSet, SIGUSR1));
        _exit(EXIT_SUCCESS);

    default:
        if (waitpid(childPid, NULL, 0) == -1)
            die("waitpid");

        /* 父进程自己再看一眼：raise(SIGUSR1) 那个信号仍然挂在自己身上 */
        if (sigpending(&pendingSet) == -1)
            die("sigpending");
        printf("父进程 fork 之后：pending 中 SIGUSR1 = %d（自己的还留着）\n",
                sigismember(&pendingSet, SIGUSR1));
        _exit(EXIT_SUCCESS);
    }
}
