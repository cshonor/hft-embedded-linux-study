/* Ch7 §7.2 — alloca：栈上分配，函数返回才回收
 * 编译: gcc -O2 -Wall -Wextra -o c7_7_alloca c7_7_alloca.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <alloca.h>
#include <sys/resource.h>
#include <sys/wait.h>

static volatile char g_sink;

/* ── A. alloca 的地址落在哪个区 ── */
static void where_is_alloca(void)
{
    char local[16];
    char *h = malloc(16);
    char *s = alloca(16);

    printf("A. 栈上局部变量 local = %p\n", (void *) local);
    printf("   alloca(16)        = %p   ← 与 local 同区，即栈\n", (void *) s);
    printf("   堆上 malloc(16)   = %p   (与 local 相距 %ld 字节)\n",
           (void *) h, (long) ((char *) h - (char *) local));
    printf("   alloca 与 local 相距 %ld 字节\n\n", (long) (s - local));
    free(h);
}

/* ── B. 循环里 alloca：不会在每次迭代结束回收 ── */
static void alloca_in_loop(void)
{
    printf("B. 循环里 alloca(4096) 五次：\n");
    char *prev = NULL;
    for (int i = 0; i < 5; i++) {
        char *p = alloca(4096);
        memset(p, i, 4096);             /* 防优化：真去碰这块内存 */
        printf("   i=%d  p=%p  与前次相差 %+ld\n", i, (void *) p,
               prev ? (long) (p - prev) : 0L);
        prev = p;
    }
    printf("   → 差值是个固定量：上一轮的块没有被回收。\n");
    printf("     alloca 只挪 sp，回收要等整个函数返回时一次性弹回。\n");
    printf("     （栈向低地址增长，所以差值是负的；多出的部分是对齐开销。）\n\n");
}

/* ── C. alloca 超大 → 直接 SIGSEGV，没有任何返回值可检查 ── */
static void alloca_huge(void)
{
    size_t mb = 16;                     /* 默认栈只有 8MB */

    printf("C. alloca(%zuMB)，栈只有 8MB ...\n", mb);
    fflush(stdout);

    size_t bytes = mb * 1024 * 1024;
    char *p = alloca(bytes);

    /* 逐页用 volatile 写：否则编译器会把「只写不读」的这段当死代码删掉，
     * 一页都不会真碰到，栈也就永远溢不出去。 */
    volatile char *vp = p;
    for (size_t i = 0; i < bytes; i += 4096)
        vp[i] = 'x';
    g_sink = vp[0];
    printf("   竟然没崩\n");
}

/* ── D. VLA 与 alloca 都在栈上，方向相反 ── */
static void vla_demo(int n)
{
    char vla[n];                        /* C99 VLA，同样在栈上 */
    memset(vla, 'v', sizeof vla);
    char *a = alloca(sizeof vla);
    printf("D. VLA[%d] = %p, alloca 同尺寸 = %p, 相差 %ld\n",
           n, (void *) vla, (void *) a, (long) (a - vla));
    printf("   （都落在栈上；栈向低地址增长，所以后分配的在更低处）\n\n");
}

int main(void)
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        if (rl.rlim_cur == RLIM_INFINITY)
            printf("本进程 RLIMIT_STACK = 不限制\n\n");
        else
            printf("本进程 RLIMIT_STACK = %lu MB\n\n",
                   (unsigned long) (rl.rlim_cur / 1024 / 1024));
    }

    where_is_alloca();
    alloca_in_loop();
    vla_demo(256);

    /* C 段放子进程里跑，父进程负责报告死因 */
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) { alloca_huge(); return 0; }

    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st))
        printf("C. 子进程被信号 %d (%s) 杀死 → 栈溢出没有「优雅失败」的途径\n",
               WTERMSIG(st), WTERMSIG(st) == SIGSEGV ? "SIGSEGV" : "?");
    else
        printf("C. 子进程 exit = %d\n", WEXITSTATUS(st));
    return 0;
}
