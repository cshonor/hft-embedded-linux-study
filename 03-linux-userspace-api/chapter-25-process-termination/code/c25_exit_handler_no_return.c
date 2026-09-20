/* c25_exit_handler_no_return.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 §25.3「Registering exit handlers」里有两条相邻、但后果完全不同的说法：

     (1) "if one of the exit handlers fails to return—either because it called
          _exit() or because the process was terminated by a signal—then the
          remaining exit handlers are not called. In addition, the remaining
          actions that would normally be performed by exit() (i.e., flushing
          stdio buffers) are not performed."

     (2) "SUSv3 states that if an exit handler itself calls exit(), the results
          are undefined. On Linux, the remaining exit handlers are invoked as
          normal."

   同样的「在 handler 里终止进程」，一个把剩下的全部取消，一个照常继续。
   本程序把两种情形各跑一遍（都放在子进程里跑，父进程只负责收尸和报数），
   让差别变成可观察量。

   ⚠️ 注意：父进程在每次 fork() 之前显式 fflush(stdout)。不这么做的话，
   父进程自己上一阶段的报表还在缓冲里，会被子进程连缓冲一起继承，
   然后被子进程的 exit() 再 flush 一遍 —— 那正是 §25.4 要讲的坑。

   编译: gcc -O0 -Wall -Wextra -o c25_exit_handler_no_return c25_exit_handler_no_return.c
   运行: ./c25_exit_handler_no_return
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void
w(const char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

static void
w2(const char *tag, const char *s)
{
    w("    ");
    w(tag);
    w(" ");
    w(s);
    w("\n");
}

/* 先注册、后跑：它只在「剩下的 handler 还会被调用」时才有输出 */
static void
handlerA(void)
{
    w2("W-hA", "       （hA：只 write + printf，不终止）");
    printf("    M-hA         （hA 的 printf，进用户态缓冲）\n");
}

/* 后注册、先跑：情形 ① —— handler 里 _exit() */
static void
handlerBAbort(void)
{
    w2("W-hB(_exit)", "（hB：立刻 _exit(7)，不再返回）");
    _exit(7);
}

/* 后注册、先跑：情形 ② —— handler 里 exit() */
static void
handlerBExit(void)
{
    w2("W-hB(exit)", " （hB：调用 exit(9)）");
    exit(9);
}

static void
runPhase(const char *tag, const char *note, void (*hB)(void))
{
    pid_t pid;
    int status;

    printf("\n=== 情形 %s：%s ===\n", tag, note);
    printf("  子进程登记顺序：先 hA，后 hB ⇒ LIFO ⇒ 先跑 hB\n");

    fflush(stdout);              /* ★ 关键：别把父进程的缓冲带进子进程 */

    pid = fork();
    if (pid == -1) {
        w("fork 失败\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        /* ---- 子进程 ---- */
        w("    W-main-child （子进程第 1 行，write ⇒ 一定看得见）\n");
        printf("    M-main-child （子进程的 printf ⇒ 只有 exit() 的 flush 才能看见它）\n");

        if (atexit(handlerA) != 0 || atexit(hB) != 0) {
            w("atexit 注册失败\n");
            _exit(1);
        }
        w("    ---- 即将 exit(3)，然后交给 exit handler ----\n");
        exit(3);                 /* 外层状态：如果 handler 不提前终止，就是它 */
        /* 不会到这里 */
    }

    /* ---- 父进程 ---- */
    if (waitpid(pid, &status, 0) == -1) {
        w("waitpid 失败\n");
        exit(EXIT_FAILURE);
    }
    printf("  [父进程] 子进程 status=%d (0x%04x)  WIFEXITED=%d  WEXITSTATUS=%d\n",
            status, (unsigned) status,
            WIFEXITED(status) ? 1 : 0,
            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

int
main(void)
{
    w("=== exit handler 里「不返回」的两种后果（原书 §25.3 相邻两段） ===\n");
    w("判断依据：M-main-child / M-hA 这两行是 printf 写的，只可能由 flush 带出来\n");

    runPhase("A", "handler 里调用 _exit(7)  ⇒  剩下的 handler 与 flush 全部取消",
             handlerBAbort);

    runPhase("B", "handler 里调用 exit(9)   ⇒  Linux 上剩下的 handler 照常跑（SUSv3 说 UB）",
             handlerBExit);

    printf("\n=== 对照结论 ===\n");
    printf("  情形 A：只有 W-main-child / W-hB(_exit) 两行 W*，**两行 M* 全部丢失**\n");
    printf("          ⇒ 既没跑 hA，也没 flush；父进程拿到 7（内层 _exit 覆盖外层 exit(3)）\n");
    printf("  情形 B：W-hA 与两行 M* 都出来了 ⇒ 剩下的 handler 与 flush 都照常\n");
    printf("          ⇒ 父进程拿到 9（内层 exit(9) 覆盖外层 exit(3)）\n");
    return EXIT_SUCCESS;
}
