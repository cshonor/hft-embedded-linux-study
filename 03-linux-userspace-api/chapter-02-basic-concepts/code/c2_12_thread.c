/* TLPI 第 2 章 §2.12 —— 线程：同一进程里共享地址空间的多条执行流
 *
 * 编译：gcc -O0 -Wall -Wextra -pthread c2_12_thread.c -o c2_12
 * 运行：./c2_12
 *
 * 本节要钉死的事实：
 *   ① 线程 = 同一进程内的多条执行流：共享地址空间、fd 表、信号处置；
 *      各自有独立的栈、寄存器、tid、errno。
 *   ② 对比 §2.7：fork 出的进程改全局变量互相看不见；线程改的立刻互相可见。
 *   ③ pthread_create 的第 4 个参数是 void*，返回值也走 pthread_join 的出口参数。
 *   ④ 编译要带 -pthread（既开宏，也把 libpthread 链进来；新版 glibc 已并入 libc）。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

static int g_shared = 0;                 /* 全局变量：所有线程共享 */
static int g_from_thread = -1;

/* 线程函数的入口签名是固定的：void *(*)(void *) */
static void *worker(void *arg)
{
    int local_in_thread = 42;            /* 局部变量：在**线程自己的栈**上 */
    printf("  [thread] pid=%d  tid=%ld  arg=\"%s\"\n",
           (int)getpid(), (long)gettid(), (const char *)arg);
    printf("  [thread] &local_in_thread = %p   （线程自己的栈）\n",
           (void *)&local_in_thread);

    g_shared = 999;                      /* 改全局变量 */
    g_from_thread = (int)(long)gettid();

    /* 通过 return 把结果交回 pthread_join 的第二个参数 */
    return (void *)(long)(local_in_thread + 1);
}

int main(void)
{
    int local_in_main = 7;

    printf("=== ① 线程与进程：pid 相同、tid 不同 ===\n");
    printf("  [main]   pid=%d  tid=%ld  &local_in_main = %p\n",
           (int)getpid(), (long)gettid(), (void *)&local_in_main);
    printf("  -> 同一个 getpid()，但 gettid() 不一样：内核视角里线程是独立调度实体\n");

    printf("\n=== ② pthread_create / pthread_join ===\n");
    printf("  创建前 g_shared = %d\n", g_shared);

    pthread_t tid;
    fflush(NULL);                        /* 多线程下更要养成清缓冲的习惯 */
    int rc = pthread_create(&tid, NULL, worker, (void *)"argument-from-main");
    if (rc != 0) {
        /* ⚠️ pthread 系列**不走 errno**，错误码直接 return */
        printf("  pthread_create 失败：%d (%s)\n", rc, strerror(rc));
        return 1;
    }
    printf("  pthread_create 返回 %d，tid（不透明句柄）= %lu\n", rc, (unsigned long)tid);

    void *ret = NULL;
    rc = pthread_join(tid, &ret);        /* 等线程结束，并取它的返回值 */
    printf("  pthread_join 返回 %d，线程返回值 = %ld\n", rc, (long)ret);

    printf("\n=== ③ 共享 vs 私有 ===\n");
    printf("  g_shared      现在 = %d   <- 线程改的，主线程直接看得见\n", g_shared);
    printf("  g_from_thread 现在 = %d   (线程的 tid)\n", g_from_thread);
    printf("  -> 对比 §2.7 的 c2_7_fork.c：fork 出的子进程改全局变量，\n");
    printf("     父进程完全看不到。这就是「线程共享地址空间」最直接的证据。\n");

    printf("\n=== ④ 四线程并发：看 tid 与栈地址 ===\n");
    pthread_t ts[4];
    for (long i = 0; i < 4; i++) {
        rc = pthread_create(&ts[i], NULL, worker, (void *)"concurrent");
        if (rc != 0) { printf("  创建第 %ld 个失败：%s\n", i, strerror(rc)); continue; }
    }
    for (int i = 0; i < 4; i++) pthread_join(ts[i], NULL);
    printf("  -> 4 条线程的 tid 互不相同，但 getpid() 全都一样。\n");

    printf("\n=== ⑤ 线程 vs 进程 对照 ===\n");
    printf("  %-22s %-28s %s\n", "resource", "thread (in one process)", "separate process");
    printf("  %-22s %-28s %s\n", "----------------------",
           "----------------------------", "----------------------------");
    printf("  %-22s %-28s %s\n", "address space", "SHARED", "separate (COW after fork)");
    printf("  %-22s %-28s %s\n", "stack", "one per thread", "one per process");
    printf("  %-22s %-28s %s\n", "file descriptor table", "SHARED", "inherited copy");
    printf("  %-22s %-28s %s\n", "signal disposition", "SHARED", "inherited copy");
    printf("  %-22s %-28s %s\n", "creation cost", "low (no new address space)", "higher (page tables etc.)");
    printf("  %-22s %-28s %s\n", "who is scheduled", "each thread is", "each process is");
    printf("  %-22s %-28s %s\n", "crash blast radius", "kills the whole process", "kills only that process");
    printf("\n  HFT 读法：线程省掉了地址空间切换，但共享状态就要同步；\n");
    printf("  「一个进程一个核 + 无锁环形队列」往往是比多线程更稳的路径。\n");
    return 0;
}
