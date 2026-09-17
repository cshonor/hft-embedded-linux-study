/* probe24.c

   ⚠️ 本仓库自写 —— 探针，**不属于原书内容**。
      每开一章先建它：把笔记里要引用的「CE 沙箱事实」一次性问清楚，
      免得正文里出现凭印象写的数字。

   本探针回答的问题：
     1. pid / ppid 为什么这么小（容器 PID namespace）
     2. PATH 到底有没有（`execlp()` 找不到 echo 的原因）
     3. 哪些绝对路径的程序真的存在且可执行（给 exec 演示挑一条确定的路径）
     4. 进程数/描述符这些 "限制" 的真实取值（fork 能开多少）
     5. RLIMIT_CORE —— 决定习题 24-3（"怎么在某一刻拿到 core dump"）在沙箱里能不能演示

   编译（自包含）：
     gcc -O0 -Wall -Wextra -o probe24 probe24.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

int main(void)
{
    const char *cands[] = {"/bin/echo", "/usr/bin/echo", "/bin/sh",
                           "/usr/bin/sh", "/bin/cat", "/proc/self/exe", NULL};
    struct rlimit rl;
    int i;

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[身份] pid=%ld ppid=%ld\n", (long) getpid(), (long) getppid());

    {
        const char *p = getenv("PATH");
        printf("[PATH] %s\n", (p == NULL) ? "(未设置)" : p);
    }

    for (i = 0; cands[i] != NULL; i++)
        printf("[access X_OK] %-16s = %d   （0 = 存在且可执行）\n",
                cands[i], access(cands[i], X_OK));

    printf("[limits] _SC_OPEN_MAX         = %ld\n", sysconf(_SC_OPEN_MAX));
    printf("[limits] _SC_CHILD_MAX        = %ld\n", sysconf(_SC_CHILD_MAX));
    printf("[limits] _SC_NPROCESSORS_ONLN = %ld\n", sysconf(_SC_NPROCESSORS_ONLN));

    if (getrlimit(RLIMIT_NPROC, &rl) == 0)
        printf("[rlimit] RLIMIT_NPROC = %ld / %ld\n",
                (long) rl.rlim_cur, (long) rl.rlim_max);
    if (getrlimit(RLIMIT_CORE, &rl) == 0)
        printf("[rlimit] RLIMIT_CORE  = %ld / %ld   （0 = 不会写 core 文件）\n",
                (long) rl.rlim_cur, (long) rl.rlim_max);
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        printf("[rlimit] RLIMIT_NOFILE = %ld / %ld\n",
                (long) rl.rlim_cur, (long) rl.rlim_max);

    return 0;
}
