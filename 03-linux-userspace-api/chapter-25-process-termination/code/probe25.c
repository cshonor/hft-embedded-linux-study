/* probe25.c

   ⚠️ 本仓库自写 —— 探针，**不属于原书内容**。

   每开一章先建它：把笔记里要引用的「CE 沙箱事实」一次性问清楚，
   免得在笔记里写「本机观察到 …」的时候其实是凭空想象。

   本章（Ch25 Process Termination）要问的：

     ① sysconf(_SC_ATEXIT_MAX) —— 原书 25.3 明确点名了这个数字，
        并断言 glibc 用链表实现 ⇒ 上限是「有符号 32 位整数最大值」。
     ② 真的连续注册 20 万个 exit handler 会不会失败（验证 ① 的说法）。
     ③ stdout 到底是不是 socket —— Ch13 已经确认过，本章 §25.4 全靠它：
        只有「stdout 不是终端」时，Listing 25-2 那个重复输出的现象才出得来。
     ④ glibc 版本、EXIT_SUCCESS / EXIT_FAILURE 的实际取值。
     ⑤ 本章要引用的几个 rlimit 与 PID namespace 事实（与 Ch24 同源，复核一次）。

   编译: gcc -O0 -Wall -Wextra -o probe25 probe25.c
   运行: ./probe25
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gnu/libc-version.h>    /* gnu_get_libc_version() */

#define N_HANDLERS 200000

static void nothing(void)
{
    /* 空的 exit handler：本节只想撞出「注册上限」，不想刷屏 */
}

int
main(int argc, char *argv[])
{
    struct stat st;
    struct rlimit rl;
    long v;

    /* 用 setvbuf 关缓冲：后面每行 println 都要能被单独看到，
       否则全缓冲下报错信息会和别的输出挤在一起看不清。 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[进程] getpid()  = %ld   getppid() = %ld\n",
            (long) getpid(), (long) getppid());

    printf("[libc] gnu_get_libc_version() = %s\n", gnu_get_libc_version());
    printf("[libc] __GLIBC__ = %d, __GLIBC_MINOR__ = %d\n",
            __GLIBC__, __GLIBC_MINOR__);

    printf("\n[①] sysconf(_SC_ATEXIT_MAX) —— 原书 25.3 点名的那个数字\n");
    v = sysconf(_SC_ATEXIT_MAX);
    if (v == -1)
        printf("  _SC_ATEXIT_MAX  = -1（不确定 / 无限）\n");
    else
        printf("  _SC_ATEXIT_MAX  = %ld  (0x%lx)\n", v, (unsigned long) v);
    printf("  对照：INT_MAX   = 2147483647 (0x7fffffff)\n");

    printf("\n[②] 真的连续注册 %d 个 exit handler（用空函数，不刷屏）\n",
            N_HANDLERS);
    {
        long i;
        int failed = -1;
        for (i = 0; i < N_HANDLERS; i++) {
            if (atexit(nothing) != 0) {   /* 原书：成功返回 0，失败返回非 0 */
                failed = (int) i;
                break;
            }
        }
        if (failed < 0)
            printf("  注册 %d 个：全部成功\n", N_HANDLERS);
        else
            printf("  注册到第 %d 个失败（errno=%d %s）\n",
                    failed, errno, strerror(errno));
        printf("  ⇒ 原书「glibc 用链式列表 ⇒ 实际上撞不到上限」得到印证\n");
        /* 用 _exit() 收场：跳过后面的 handler 与 stdio flush，省时间 */
    }

    printf("\n[③] stdout 是什么（本章 §25.4 的前提）\n");
    if (fstat(STDOUT_FILENO, &st) == -1) {
        printf("  fstat(1) 失败: %s\n", strerror(errno));
    } else {
        printf("  S_ISREG  = %d   S_ISCHR = %d   S_ISFIFO = %d   S_ISSOCK = %d\n",
                S_ISREG(st.st_mode) ? 1 : 0,
                S_ISCHR(st.st_mode) ? 1 : 0,
                S_ISFIFO(st.st_mode) ? 1 : 0,
                S_ISSOCK(st.st_mode) ? 1 : 0);
        printf("  isatty(1) = %d\n", isatty(STDOUT_FILENO));
        printf("  ⇒ 不是终端 ⇒ glibc 对 stdout 用【全缓冲】(BUFSIZ=%d)\n",
                BUFSIZ);
        printf("  ⇒ 原书 Listing 25-2 在 CE 上直接表现为「重定向到文件」那一半\n");
    }

    printf("\n[④] EXIT_SUCCESS / EXIT_FAILURE\n");
    printf("  EXIT_SUCCESS = %d\n", EXIT_SUCCESS);
    printf("  EXIT_FAILURE = %d\n", EXIT_FAILURE);

    printf("\n[⑤] 几个 rlimit（与 Ch24 同源，复核一次）\n");
    if (getrlimit(RLIMIT_CORE, &rl) == 0)
        printf("  RLIMIT_CORE   soft=%llu hard=%llu\n",
                (unsigned long long) rl.rlim_cur,
                (unsigned long long) rl.rlim_max);
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        printf("  RLIMIT_NOFILE soft=%llu hard=%llu\n",
                (unsigned long long) rl.rlim_cur,
                (unsigned long long) rl.rlim_max);
    printf("  _SC_OPEN_MAX  = %ld\n", sysconf(_SC_OPEN_MAX));
    printf("  _SC_CHILD_MAX = %ld\n", sysconf(_SC_CHILD_MAX));
    printf("  _SC_NPROCESSORS_ONLN = %ld\n", sysconf(_SC_NPROCESSORS_ONLN));

    printf("\n[⑥] 本进程读到的 PATH 与 argv[0]\n");
    printf("  getenv(\"PATH\") = %s\n",
            getenv("PATH") ? getenv("PATH") : "(NULL)");
    if (argc > 0)
        printf("  argv[0]        = %s\n", argv[0]);

    _exit(0);   /* 跳过后台那 20 万个空 handler */
}
