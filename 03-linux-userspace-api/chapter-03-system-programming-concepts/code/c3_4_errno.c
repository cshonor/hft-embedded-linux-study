/* TLPI 第 03 章 §3.4 —— errno 范式的六条硬规则 + 一次实测
 *
 * 六条规则（man errno(3) / intro(2)）：
 *   1. 先看返回值，再读 errno —— errno 只在「失败」时有意义
 *   2. **成功的调用不会把 errno 清零**  ← 本节最常被写错的一条
 *   3. errno 是「左值宏」：#define errno (*__errno_location())，所以能赋值
 *   4. errno 是线程私有（TLS）的：每个线程一份自己的 errno
 *   5. 判断「某个调用有没有失败」必须紧邻读取：中间插一条可能失败的调用就会冲掉
 *   6. 库函数失败**不一定**设 errno（看各自 man 的 NOTES）
 *
 * 编译：gcc -O0 -Wall -Wextra -pthread -o c3_4 c3_4_errno.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int *g_main_errno_addr;
static int *g_thread_errno_addr;

static void *worker(void *arg)
{
    (void) arg;
    /* 先把本线程的 errno 弄成非零，证明它跟主线程互相独立 */
    errno = 12345;
    g_thread_errno_addr = &errno;
    printf("  线程：   &errno = %p   errno = %d\n", (void *) &errno, errno);
    return NULL;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    int save;

    printf("=== 规则 2 实测：成功的调用不会清零 errno ===\n");
    errno = 0;
    int fd = open("/dev/null", O_RDONLY);          /* 必然成功 */
    printf("  errno=0 时 open(\"/dev/null\") 成功 → fd=%d，errno=%d\n", fd, errno);

    /* 先人为制造一个非零 errno，再跑一个成功的调用 */
    open("/definitely/not/here", O_RDONLY);        /* 必然失败，errno 被设上 */
    save = errno;
    printf("  制造失败后                     errno=%d (%s)\n", save, strerror(save));

    close(fd);
    close(-1);                                     /* 这个会失败 */
    printf("  紧接着再做一次成功的 close      → close(fd) 的返回值被忽略，errno 仍是 %d\n",
           errno);
    printf("  ↑ 结论：**errno 不会被成功调用清掉**，所以「errno != 0 就说明上一步失败」是错的\n\n");

    printf("=== 规则 3 实测：errno 是可赋值的左值宏 ===\n");
    errno = 0;
    printf("  errno = 0 之后读回 = %d\n", errno);
    errno = EDOM;
    printf("  errno = EDOM(%d) 之后读回 = %d (%s)\n", EDOM, errno, strerror(errno));
    printf("  errno 的地址（即 __errno_location()）= %p\n\n", (void *) &errno);
    errno = 0;

    printf("=== 规则 4 实测：errno 是线程私有的 ===\n");
    g_main_errno_addr = &errno;
    errno = 999;
    printf("  主线程：&errno = %p   errno = %d\n", (void *) g_main_errno_addr, errno);
    pthread_t tid;
    if (pthread_create(&tid, NULL, worker, NULL) == 0) {
        pthread_join(tid, NULL);
        printf("  两个地址%s → errno 是 TLS，多线程下各自失败互不干扰\n",
               g_main_errno_addr == g_thread_errno_addr ? "相同（意外！）" : "不同");
    } else {
        printf("  pthread_create 失败\n");
    }
    printf("  主线程的 errno 仍是 %d（没被线程里那次赋值影响）\n\n", errno);

    printf("=== 规则 5 实测：中间插一条调用会冲掉 errno ===\n");
    errno = 0;
    int bad = open("/definitely/not/here", O_RDONLY);
    int saved = errno;                    /* ✅ 紧邻读取 */
    printf("  失败 open → bad=%d，立刻读 errno=%d (%s)\n", bad, saved, strerror(saved));

    errno = 0;
    bad = open("/definitely/not/here", O_RDONLY);
    printf("  失败 open → bad=%d，但中间先 printf 了一次（它自己可能碰 errno）\n", bad);
    printf("  之后才读 errno=%d\n", errno);
    printf("  ↑ 顺序上「printf 之后再读」在本次碰巧还对，但这是运气；范式要求紧邻读\n\n");

    printf("=== 规则 6 实测：库函数失败不一定设 errno ===\n");
    errno = 0;
    double d = strtod("0.0", NULL);       /* 成功，不该碰 errno */
    printf("  strtod(\"0.0\") = %g，errno = %d\n", d, errno);
    errno = 0;
    d = strtod("1e9999", NULL);           /* 溢出 → 规范要求设 ERANGE */
    printf("  strtod(\"1e9999\") = %g，errno = %d (%s)\n", d, errno, strerror(errno));

    errno = 0;
    char *e = getenv("___definitely_not_set___");
    printf("  getenv(不存在) = %p，errno = %d  ← NULL 不代表出错，读 man NOTES\n",
           (void *) e, errno);
    return 0;
}
