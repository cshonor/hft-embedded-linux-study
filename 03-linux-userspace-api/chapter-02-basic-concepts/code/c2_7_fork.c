/* TLPI 第 2 章 §2.7 —— 进程：fork 的返回值、独立地址空间、僵尸进程
 *
 * 编译：gcc -O0 -Wall -Wextra c2_7_fork.c -o c2_7
 * 运行：./c2_7
 *
 * 本节要钉死的事实：
 *   ① fork() 一次调用、两次返回：父进程拿到子 PID，子进程拿到 0，失败返回 -1。
 *   ② 父子各有独立虚拟地址空间 —— 全局变量的修改互不影响（写时复制）。
 *      「虚拟地址相同」不等于「同一块物理内存」。
 *   ③ 子进程退出但父进程没 wait，子进程就停在 Z（zombie）状态占着 PID。
 *   ④ _exit() 不 flush stdio；exit() 会。混用 printf + _exit 会丢输出。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>

static int g_counter = 100;             /* 全局变量：放进数据段 */

/* 读 /proc/<pid>/stat 的第 3 个字段（进程状态字符） */
static char proc_state(pid_t pid)
{
    char path[64], buf[512];
    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return '?';
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return '?';
    buf[n] = '\0';
    /* 格式：pid (comm) state ...  —— comm 里可能有空格和括号，从右括号后面找 */
    char *rp = strrchr(buf, ')');
    if (!rp || rp[1] == '\0') return '?';
    return rp[2];                        /* ')' 后面是空格，再后面是状态字符 */
}

static const char *state_name(char c)
{
    switch (c) {
    case 'R': return "Running";
    case 'S': return "Sleeping";
    case 'D': return "Uninterruptible sleep";
    case 'T': return "Stopped";
    case 'Z': return "Zombie";
    case 'X': return "Dead";
    default:  return "?";
    }
}

int main(void)
{
    printf("=== ① fork 一次调用、两次返回 ===\n");
    printf("  fork 之前：只有父进程，pid=%d\n", (int)getpid());
    fflush(NULL);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        printf("  子进程：fork() 返回 0        → 我用它判断「我是孩子」\n");
        printf("          我的 pid=%d  ppid=%d\n", (int)getpid(), (int)getppid());
        return 0;
    } else {
        printf("  父进程：fork() 返回 %d  → 这就是子进程的 PID\n", (int)pid);
        printf("          我的 pid=%d\n", (int)getpid());
        int st; waitpid(pid, &st, 0);
        printf("  父进程：子进程退出码 = %d\n", WEXITSTATUS(st));
    }

    printf("\n=== ② 独立地址空间（写时复制）===\n");
    printf("  fork 之前 g_counter = %d，&g_counter = %p\n", g_counter, (void *)&g_counter);
    fflush(NULL);

    pid_t p2 = fork();
    if (p2 == 0) {
        printf("  [child]  进来时  g_counter = %d  &g_counter = %p\n",
               g_counter, (void *)&g_counter);
        g_counter = 999;
        printf("  [child]  改成 999 后 g_counter = %d\n", g_counter);
        fflush(NULL);                    /* 必须先 flush：_exit() 不会替你清 stdio */
        _exit(0);
    }
    int st2; waitpid(p2, &st2, 0);
    printf("  [parent] 子进程改完退出后，g_counter = %d  &g_counter = %p\n",
           g_counter, (void *)&g_counter);
    printf("  → 父进程看到的还是 %d：两个进程的 g_counter 是两份独立的内存。\n",
           100);
    printf("  → 若两行的 &g_counter 地址相同，说明「虚拟地址一样」也**不代表**\n");
    printf("     同一块物理内存 —— 页表把它们映射到了不同的物理页（写时复制）。\n");

    printf("\n=== ③ 僵尸进程：退出但不被收尸 ===\n");
    fflush(NULL);
    pid_t p3 = fork();
    if (p3 == 0) _exit(5);               /* 子进程立刻退出 */

    /* 父进程故意先不 wait，让子进程停在 Z 状态 */
    struct timespec tv = { 0, 50000000L };   /* 50ms，给它时间变成僵尸 */
    nanosleep(&tv, NULL);
    char c = proc_state(p3);
    printf("  子进程 %d 已 _exit(5)，父进程还没 wait\n", (int)p3);
    printf("  /proc/%d/stat 的状态字段 = '%c' (%s)\n", (int)p3, c, state_name(c));
    if (c == 'Z')
        printf("  → 确认是僵尸：进程的「遗体」还占着一个 PID，等父进程收尸\n");

    int st3 = 0;
    pid_t got = waitpid(p3, &st3, 0);
    printf("  waitpid(%d) 返回 %d，WIFEXITED=%d WEXITSTATUS=%d  → 收尸完成\n",
           (int)p3, (int)got, WIFEXITED(st3), WEXITSTATUS(st3));
    errno = 0;
    printf("  收尸后再读 /proc/%d/stat：", (int)p3);
    char c2 = proc_state(p3);
    printf(" 状态='%c'\n", c2);
    printf("  → 变成 '?'（文件已消失，open 返回 ENOENT）说明进程表项真的释放了\n");

    printf("\n=== ④ _exit() 不 flush stdio ===\n");
    fflush(NULL);
    pid_t p4 = fork();
    if (p4 == 0) {
        printf("  [子A] 这行用 printf 写入缓冲，然后调 _exit(0) → 应该看不到\n");
        _exit(0);
    }
    waitpid(p4, NULL, 0);
    pid_t p5 = fork();
    if (p5 == 0) {
        printf("  [子B] 这行用 printf 写入缓冲，然后调 exit(0)  → 应该看得到\n");
        exit(0);
    }
    waitpid(p5, NULL, 0);
    fflush(NULL);
    printf("  → 上面只看到子B，证明 _exit() 绕过 stdio 清理直接进内核。\n");
    printf("  → 所以在 fork 前要 fflush(NULL)，在子进程里该用 _exit 就用 _exit，\n");
    printf("     否则同一条缓冲会被父、子各输出一遍（经典重复打印 bug）。\n");

    printf("\n=== ⑤ fork 前不 fflush：同一行会被打印两遍 ===\n");
    printf("  [A] 这行先 fflush 出去，作为对照组\n");
    fflush(NULL);
    printf("  [B] 这行留在缓冲里，然后**故意不 flush** 就 fork\n");
    pid_t p6 = fork();
    if (p6 == 0) {
        exit(0);                         /* exit() 会 flush：子进程把继承来的 [B] 印一遍 */
    }
    waitpid(p6, NULL, 0);
    fflush(NULL);                        /* 父进程再印一遍自己的 [B] */
    printf("  -> 若 [B] 出现了两次，就是「fork 复制了未 flush 的缓冲」在起作用。\n");
    printf("  -> 修法只有一个字：fork 之前 fflush(NULL)。\n");
    printf("  -> 反过来说，_exit() 不 flush，所以子进程自己的 printf 会**全丢**：\n");
    printf("     要输出就先 fflush(NULL) 再 _exit()，或者干脆用 exit()。\n");

    printf("\n=== ⑥ 退出码与 wait 宏 ===\n");
    printf("  WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG/WCOREDUMP 是宏，\n");
    printf("  不是函数 —— 它们直接对 wait status 整数做位运算，零开销。\n");
    return 0;
}
