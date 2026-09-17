/* ex24_3_core_dump.c

   ⚠️ 本仓库自写 —— 习题 24-3 的解，**原书没有给这一题的答案，也没有配套程序**。

   习题 24-3（原书书页 530）：
     "Assuming that we can modify the program source code, how could we get a
      core dump of a process at a given moment in time, while letting the
      process continue execution?"

   思路：**fork 一个子进程，让子进程去死。**
        子进程的地址空间是父进程那一刻的副本（写时复制），所以子进程崩溃产生的
        core 文件里，就是父进程当时的完整内存映像。父进程 `wait()` 收尸后继续跑。

   本程序顺便实测三件事（这些是「能不能拿到 core」的硬前提）：
     ① 当前 RLIMIT_CORE 是多少 —— 是 0 就根本写不出 core 文件
     ② 能不能把它抬上去（非 root 提升 hard limit 会被拒）
     ③ wait() 的 status 怎么表达「被信号杀死」和「有没有 core」

   编译（自包含）：
     gcc -O0 -Wall -Wextra -o ex24_3_core_dump ex24_3_core_dump.c
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static void showFile(const char *path)
{
    char buf[256];
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        printf("  %-34s = <打不开> errno=%d (%s)\n", path, errno, strerror(errno));
        return;
    }
    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0)
        n = 0;
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = '\0';
    printf("  %-34s = \"%s\"\n", path, buf);
    close(fd);
}

static void showCoreLimit(const char *tag)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_CORE, &rl) == 0)
        printf("  %s RLIMIT_CORE = %ld / %ld   （soft / hard，单位字节；0 = 不写 core）\n",
               tag, (long) rl.rlim_cur, (long) rl.rlim_max);
}

int main(void)
{
    struct rlimit rl;
    pid_t pid;
    int status;

    setbuf(stdout, NULL);

    printf("=== ① 动手之前的状态 ===\n");
    showCoreLimit("[初始]");
    showFile("/proc/sys/kernel/core_pattern");

    printf("\n=== ② 试着把 soft 和 hard 一起抬到无限 ===\n");
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &rl) == -1)
        printf("  setrlimit(INFINITY / INFINITY) 失败：errno=%d (%s)\n",
               errno, strerror(errno));
    else
        printf("  setrlimit(INFINITY / INFINITY) 成功\n");
    showCoreLimit("[之后]");

    printf("\n=== ③ 退一步：只把 soft 抬到 hard 允许的上限 ===\n");
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        rl.rlim_cur = rl.rlim_max;      /* hard 是 0 的话，抬完还是 0 */
        if (setrlimit(RLIMIT_CORE, &rl) == -1)
            printf("  setrlimit(soft -> hard) 失败：errno=%d (%s)\n",
                   errno, strerror(errno));
        else
            printf("  setrlimit(soft -> hard) 成功（soft 现在 = hard）\n");
    }
    showCoreLimit("[之后]");

    printf("\n=== ④ 关键一步：fork 一个子进程让它崩，父进程继续跑 ===\n");
    pid = fork();
    if (pid == -1) {
        printf("  fork 失败 errno=%d (%s)\n", errno, strerror(errno));
        return 1;
    }

    if (pid == 0) {                     /* 子进程：替父进程去死 */
        printf("  子进程 pid=%ld —— 即将 abort()（SIGABRT 的默认动作是终止 + core）\n",
               (long) getpid());
        fflush(NULL);
        abort();                        /* 不返回 */
        _exit(99);                      /* 万一 abort 返回了，别继续往下跑 */
    }

    if (wait(&status) == -1) {
        printf("  wait 失败 errno=%d (%s)\n", errno, strerror(errno));
        return 1;
    }

    printf("  父进程收尸：子进程 pid=%ld  status=%d\n", (long) pid, status);
    printf("    WIFSIGNALED = %d   （1 = 子进程是被信号杀死的）\n", WIFSIGNALED(status));
    printf("    WTERMSIG    = %d   （6 = SIGABRT）\n", WTERMSIG(status));
    printf("    WCOREDUMP   = %d   （= status & 0x80：非 0 = 内核走完了 coredump 流程，"
           "但**不等于**磁盘上有 core 文件）\n", WCOREDUMP(status));

    printf("\n=== ⑤ 父进程还活着，继续往下执行 ===\n");
    printf("  父进程 pid=%ld —— 这一行能打出来，就是习题要求的"
           "「while letting the process continue execution」\n", (long) getpid());

    printf("\n=== ⑥ 找找看有没有 core 文件落盘 ===\n");
    showFile("./core");
    showFile("/core");
    printf("  access(\"./core\", F_OK) = %d   （0 = 存在）\n", access("./core", F_OK));

    return 0;
}
