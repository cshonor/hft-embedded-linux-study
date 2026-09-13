/* c6_1_process_ctx.c — 6.1 进程 = 内核管理的执行上下文（资源容器视角）
 * 编译: gcc -O2 -Wall -Wextra -o c6_1_process_ctx c6_1_process_ctx.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>

int main(void)
{
    /* 身份：内核 task_struct 里的第一组字段 */
    printf("PID  = %d\n", (int)getpid());
    printf("PPID = %d\n", (int)getppid());
    printf("PGID = %d   (进程组)\n", (int)getpgrp());
    printf("SID  = %d   (会话)\n", (int)getsid(0));
    printf("UID=%d EUID=%d  GID=%d EGID=%d\n",
           (int)getuid(), (int)geteuid(), (int)getgid(), (int)getegid());

    /* 资源二：fd 表（内核替你维护的打开文件表） */
    printf("fd 表: stdin=%d stdout=%d stderr=%d\n",
           STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);

    /* 资源三：资源限制（rlimit，进程属性） */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        printf("RLIMIT_NOFILE: cur=%lu  max=%lu\n",
               (unsigned long)rl.rlim_cur, (unsigned long)rl.rlim_max);
    if (getrlimit(RLIMIT_STACK, &rl) == 0)
        printf("RLIMIT_STACK : cur=%lu MB\n",
               (unsigned long)(rl.rlim_cur / 1024 / 1024));
    return 0;
}
