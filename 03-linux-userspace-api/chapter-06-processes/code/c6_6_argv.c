/* c6_6_argv.c — 6.6 命令行参数：argc/argv 布局 + argv[argc]==NULL + 来源(fork+execv)
 * 编译: gcc -O2 -Wall -Wextra -o c6_6_argv c6_6_argv.c
 * 用法: ./c6_6_argv -a hello --flag
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static void dump_argv(int argc, char **argv)
{
    printf("  argc = %d\n", argc);
    for (int i = 0; i <= argc; i++) {
        if (i == argc)
            printf("  argv[%d] = %-12s  <- C 标准保证的 NULL 哨兵\n",
                   i, argv[i] ? "(非NULL!)" : "NULL");
        else
            printf("  argv[%d] = %-12s (@%p)\n", i, argv[i], (void *)argv[i]);
    }
}

int main(int argc, char **argv)
{
    printf("=== A. 本进程的 argv ===\n");
    dump_argv(argc, argv);

    printf("=== B. /proc/self/cmdline（内核存的原始副本，NUL 分隔）===\n");
    FILE *f = fopen("/proc/self/cmdline", "rb");
    if (f) {
        char buf[512];
        size_t n = fread(buf, 1, sizeof buf, f);
        for (size_t i = 0; i < n; i++)
            putchar(buf[i] == '\0' ? '|' : buf[i]);
        putchar('\n');
        fclose(f);
    }

    /* 防无限递归：execv 起来的那个进程 argv[1] 是 「first」，直接收工 */
    if (argc > 1 && strcmp(argv[1], "first") == 0) {
        printf("=== C. 我是被 execv 起来的，argv 由父进程指定，不再自我重启 ===\n");
        return 0;
    }

    printf("=== C. 用 fork + execv 给另一个进程指定 argv ===\n");
    char self[512];
    ssize_t k = readlink("/proc/self/exe", self, sizeof self - 1);
    if (k <= 0) { printf("  readlink(/proc/self/exe) 失败\n"); return 0; }
    self[k] = 0;
    printf("  可执行文件真实路径 = %s\n", self);

    pid_t pid = fork();
    if (pid == 0) {
        /* argv[0] 可以是任意字符串，与真实程序名无关 —— 观察 A 段会打印 「改名了」 */
        char *newargv[] = { (char *)"改名了", (char *)"first", (char *)"-x", NULL };
        execv(self, newargv);
        perror("execv");
        _exit(127);                 /* 只有 execv 失败才会到这 */
    }
    wait(NULL);
    return 0;
}
