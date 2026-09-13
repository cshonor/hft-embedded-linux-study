/* c6_2_pid_orphan.c — 6.2 PID/PPID、子进程回收、孤儿进程（PPID 被改写）
 * 编译: gcc -O2 -Wall -Wextra -o c6_2_pid_orphan c6_2_pid_orphan.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void say(const char *tag)
{
    printf("  [%-10s] pid=%-6d ppid=%-4d\n", tag, (int)getpid(), (int)getppid());
    fflush(stdout);   /* ⚠️ 子进程随后 _exit()，不 flush 的话这段输出会丢 */
}

int main(void)
{
    say("main");

    /* ---- A. 正常父子：子进程打印时父还活着 ---- */
    printf("--- A. fork 后父子各自的身份 ---\n");
    pid_t c = fork();
    if (c < 0) { perror("fork"); return 1; }
    if (c == 0) { say("child"); _exit(0); }

    int st = 0;
    pid_t w = wait(&st);
    printf("  wait 返回 %d（就是子进程 PID）；WIFEXITED=%d WEXITSTATUS=%d\n",
           (int)w, WIFEXITED(st), WEXITSTATUS(st));

    /* ---- B. 孤儿：让「中间父」先退，孙子的 PPID 被内核改写 ---- */
    printf("--- B. 中间父退出 -> 孙子变孤儿，PPID 被改写 ---\n");
    c = fork();
    if (c < 0) { perror("fork"); return 1; }
    if (c == 0) {
        pid_t g = fork();
        if (g < 0) _exit(1);
        if (g == 0) {                     /* 孙子 */
            sleep(1);                     /* 等中间父退出 */
            say("grandchild");
            printf("             ↑ 中间父已退出，PPID 不再是它，而是被 1 号进程收养\n");
            fflush(stdout);
            _exit(0);
        }
        _exit(0);                          /* 中间父立刻退出 -> 孙子成孤儿 */
    }
    wait(NULL);                            /* 只收得到中间父 */

    /* main 自己多活一会儿，等孙子把话说完（CE 沙箱里 main 一退就收走全部子进程） */
    sleep(2);
    printf("  [main      ] 收工（pid=%d）\n", (int)getpid());
    return 0;
}
