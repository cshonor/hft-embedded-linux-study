/* TLPI 第 2 章 §2.2 —— 迷你 shell：read → fork → exec → wait 循环
 *
 * 编译：gcc -O0 -Wall -Wextra c2_2_minishell.c -o c2_2
 * 运行：printf 'self\nnosuchcmd\nbuiltin\nsys\nhelp\nexit\n' | ./c2_2
 *
 * 这个 shell 只认 4 个内置命令（不依赖外部命令，便于在最小容器里跑）：
 *   self        fork + execve(/proc/self/exe) 自举一个新程序  → 成功路径
 *   nosuchcmd   execvp 一个不存在的命令                       → 经典 exit 127 路径
 *   sys         system("true") 对照：验证它必须依赖 /bin/sh
 *   help/exit   打印帮助 / 退出
 *
 * 本节要钉死的事实：
 *   ① shell 是普通用户态进程，核心就是「读 → fork → exec → wait」。
 *   ② 内置命令不 fork（要改 shell 自身状态），外部命令必须 fork+exec。
 *   ③ exec 成功就永不返回；走到 perror 一定是失败，惯例 _exit(127)。
 *   ④ system() 是隐式的 fork + /bin/sh -c + wait，没有 /bin/sh 就整个失败。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAXLINE 512

/* 把 wait status 翻译成人话 */
static void report_status(const char *tag, int status)
{
    if (WIFEXITED(status))
        printf("  %s: 正常退出 WEXITSTATUS=%d\n", tag, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("  %s: 被信号杀死 WTERMSIG=%d (%s)%s\n", tag, WTERMSIG(status),
               strsignal(WTERMSIG(status)),
               WCOREDUMP(status) ? " + core" : "");
    else if (WIFSTOPPED(status))
        printf("  %s: 被暂停 WSTOPSIG=%d\n", tag, WSTOPSIG(status));
    else
        printf("  %s: 其他 status=0x%x\n", tag, status);
}

/* 外部命令路径：fork + exec + wait */
static void run_external(const char *cmd)
{
    fflush(NULL);                       /* fork 前清缓冲，否则子进程会重复输出 */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        /* ---- 子进程：把自己替换成目标程序 ---- */
        if (strcmp(cmd, "self") == 0) {
            static char exe[512];
            ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
            if (n < 0) { perror("readlink"); _exit(127); }
            exe[n] = '\0';
            char *av[] = { exe, "--child", NULL };
            char *ev[] = { "C2_ROLE=child", NULL };
            printf("  [child] 即将 execve 自己：%s\n", exe);
            fflush(NULL);
            execve(exe, av, ev);
            perror("execve");           /* 只有失败才会走到这里 */
            _exit(127);
        }
        /* nosuchcmd：走 execvp，让它去 PATH 里搜 */
        printf("  [child] execvp(\"%s\") ...\n", cmd);
        fflush(NULL);
        errno = 0;
        execvp(cmd, (char *[]) { (char *)cmd, NULL });
        printf("  [child] execvp 失败 errno=%d (%s) -> 惯例 _exit(127)\n",
               errno, strerror(errno));
        fflush(NULL);
        _exit(127);
    }

    /* ---- 父进程：等它 ---- */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return; }
    printf("  [parent] 子进程 %d 结束\n", (int)pid);
    report_status("child", status);
}

static void cmd_sys(void)
{
    printf("  system(\"true\") —— 它的内部是 fork + /bin/sh -c + waitpid\n");
    errno = 0;
    int rc = system("true");
    printf("  system() 返回值 = %d", rc);
    if (rc == -1)
        printf("   errno=%d (%s)\n", errno, strerror(errno));
    else
        printf("   WIFEXITED=%d WEXITSTATUS=%d\n", WIFEXITED(rc), WEXITSTATUS(rc));
    printf("  -> 本环境没有 /bin/sh，所以 system() 整条路都断了；\n");
    printf("     这正是「别在热路径依赖 system()」的最硬证据。\n");
}

int main(int argc, char **argv)
{
    /* 被 exec 出来的那个「自己」走这个分支 */
    if (argc > 1 && strcmp(argv[1], "--child") == 0) {
        printf("  [child] 我是被 execve 出来的新程序：pid=%d argv[0]=%s\n",
               (int)getpid(), argv[0]);
        printf("  [child] 环境变量 C2_ROLE=%s\n",
               getenv("C2_ROLE") ? getenv("C2_ROLE") : "(丢失)");
        return 7;                       /* 用退出码 7 给父进程一个信号 */
    }

    char line[MAXLINE];
    printf("=== 迷你 shell（TLPI §2.2）===\n");
    printf("内置：self / nosuchcmd / sys / help / exit\n\n");

    for (;;) {
        printf("$ ");
        fflush(stdout);                 /* 提示符必须立刻可见 */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n(EOF，shell 退出)\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        if (strcmp(line, "exit") == 0) { printf("bye\n"); break; }
        if (strcmp(line, "help") == 0) {
            printf("  self       fork + execve 自己（成功路径，退出码 7）\n");
            printf("  nosuchcmd  execvp 找不到命令（失败路径，exit 127）\n");
            printf("  sys        system(\"true\") 对照实验\n");
            printf("  exit       退出\n");
            continue;                   /* 内置：不 fork */
        }
        if (strcmp(line, "sys") == 0) { cmd_sys(); continue; }

        run_external(line);             /* 其余当外部命令处理 */
    }
    return 0;
}
