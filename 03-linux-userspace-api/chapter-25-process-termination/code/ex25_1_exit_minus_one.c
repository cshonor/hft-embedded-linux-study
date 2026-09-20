/* ex25_1_exit_minus_one.c

   ⚠️ 本仓库自写 —— 习题 25-1 的解，**原书没有给这一题的答案**。

   题目原文（书 p.539 §25.6 Exercise）：
     "25-1. If a child process makes the call exit(-1), what exit status
      (as returned by WEXITSTATUS()) will be seen by the parent?"

   答案：**255**。

   为什么不是 -1 也不是 65535：
     · exit(status) 的形参是 int，但原书 §25.1 明说
       "only the bottom 8 bits of status are actually made available to
        the parent"；
     · 截断发生在**系统调用入口**，不在父进程取状态的时候 ——
       Linux v6.6 kernel/exit.c:989-992（exit）与 1033-1035（exit_group）
       都是 `do_exit((error_code & 0xff) << 8)`：先取低 8 位，再左移 8 位
       换成「wait status」编码（低字节留给信号号 / core 位）。
       glibc 的 _exit() 走的就是 exit_group。
     · 于是 -1 ⇒ (0xFFFFFFFF & 0xff) << 8 = 0xFF00 = 65280，
       而 WEXITSTATUS(65280) = 0xFF = 255。

   本程序不只回答 -1，而是把一整排值都跑一遍，让「低 8 位」这条规则的
   边界（127/128/255/256/257/300/511、负数、以及被信号杀死）都能被看见。

   编译: gcc -O0 -Wall -Wextra -o ex25_1_exit_minus_one ex25_1_exit_minus_one.c
   运行: ./ex25_1_exit_minus_one
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* 故意跨过 0/127/128/255/256/257 这些边界，两侧都取值 */
static const int STATUSES[] = {
    -1, -2, -256, -1000,
    0, 1, 2, 127, 128, 255, 256, 257, 300, 511, 1000, 65535,
};
#define N_STATUS (sizeof(STATUSES) / sizeof(STATUSES[0]))

static void
showStatus(int st, int status)
{
    printf("  exit(%-6d)  内核记的 status = %-6d (0x%04x)   "
           "WIFEXITED=%d  WEXITSTATUS=%-3d   (st & 0xff) = %d\n",
            st, status, (unsigned) status,
            WIFEXITED(status) ? 1 : 0,
            WIFEXITED(status) ? WEXITSTATUS(status) : -1,
            st & 0xff);
}

int
main(void)
{
    int i;

    /* 关掉 stdio 缓冲：本程序要 fork，但父进程从 printf 自己的报表都在
       fork 之后，关缓冲可以彻底排除「父子各持一份缓冲」这个干扰 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== 习题 25-1：exit(status) 的 status 只有低 8 位会到父进程手里 ===\n\n");
    printf("每行格式：子进程调的 exit() 值 → 父进程 wait() 看到的 status（十六进制）\n"
           "           → WIFEXITED / WEXITSTATUS → 与 st & 0xff 对照\n\n");

    for (i = 0; i < (int) N_STATUS; i++) {
        int st = STATUSES[i];
        pid_t pid;
        int status;

        pid = fork();
        if (pid == -1) {
            printf("  fork 失败（第 %d 个）\n", i);
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            exit(st);            /* 题目里的写法：exit()，不是 _exit() */
            /* 不会到这里 */
        }
        if (waitpid(pid, &status, 0) == -1) {
            printf("  waitpid 失败\n");
            return EXIT_FAILURE;
        }
        showStatus(st, status);
    }

    printf("\n=== 对照：子进程被信号杀死时，低 8 位不是退出码 ===\n");
    {
        pid_t pid = fork();
        int status;

        if (pid == 0) {
            raise(SIGSEGV);      /* 默认动作：终止 + core */
            _exit(0);
        }
        if (waitpid(pid, &status, 0) == -1) {
            printf("  waitpid 失败\n");
            return EXIT_FAILURE;
        }
        printf("  raise(SIGSEGV)  status = %d (0x%04x)   "
               "WIFSIGNALED=%d  WTERMSIG=%d  WCOREDUMP=%d\n",
                status, (unsigned) status,
                WIFSIGNALED(status) ? 1 : 0,
                WIFSIGNALED(status) ? WTERMSIG(status) : -1,
                WCOREDUMP(status) ? 1 : 0);
        printf("  WIFEXITED=%d   WEXITSTATUS(原样) = %d —— 无意义，别读它\n",
                WIFEXITED(status) ? 1 : 0,
                WEXITSTATUS(status));
        printf("  ⇒ status 低 7 位是信号号 11（SIGSEGV）；最高位 0x80 是 core 标志\n"
               "     ⇒ 139 = 128 + 11，这就是 shell 里 $? 的由来\n");
        printf("  ⇒ 也正因如此，exit(128+n) 与「被信号 n 杀死」在 shell 里无法区分\n");
    }

    printf("\n⭐ 习题 25-1 的答案：exit(-1) ⇒ 父进程 WEXITSTATUS() = 255\n");
    return EXIT_SUCCESS;
}
