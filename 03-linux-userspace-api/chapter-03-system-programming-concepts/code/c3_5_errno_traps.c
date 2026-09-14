/* TLPI 第 03 章 §3.4 —— errno 六种典型误用，逐个实测打脸
 *
 * 每条误用都给出「错在哪 → 实测现象 → 正确写法」。
 * 编译：gcc -O0 -Wall -Wextra -pthread -o c3_5 c3_5_errno_traps.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *g_saved_strerror1;
static char *g_saved_strerror2;

static void *worker(void *arg)
{
    (void) arg;
    /* 误用 ⑤：多线程里共享 strerror 的静态缓冲 */
    g_saved_strerror2 = strerror(EACCES);
    return NULL;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=== 误用 1：用「errno != 0」判上一步是否失败 ===\n");
    errno = 0;
    int fd = open("/dev/null", O_RDONLY);
    printf("  成功 open 后 errno = %d（干净场景下确实是 0）\n", errno);
    close(fd);
    errno = 0;
    open("/definitely/not/here", O_RDONLY);   /* 失败，留下 ENOENT */
    fd = open("/dev/null", O_RDONLY);         /* 又成功，但 errno 不会归零 */
    printf("  失败一次再成功一次，errno = %d (%s)  → 用 errno!=0 判成功/失败必然误报\n",
           errno, strerror(errno));
    close(fd);
    printf("  正确：判返回值。open 失败才返回 -1，此时 errno 才有意义\n\n");

    printf("=== 误用 2：不查返回值，直接读 errno ===\n");
    errno = 0;
    char *p = getenv("___not_set___");
    printf("  getenv(不存在) 返回 %p，errno = %d\n", (void *) p, errno);
    printf("  → 返回 NULL 是「没找到」，不是「出错」；不查文档就报错会冤枉库函数\n\n");

    printf("=== 误用 3：把 errno 当普通全局变量跨线程传 ===\n");
    errno = 7;
    pthread_t tid;
    if (pthread_create(&tid, NULL, worker, NULL) == 0) {
        pthread_join(tid, NULL);
    }
    printf("  主线程 errno = %d（线程里设的 EACCES=%d 完全没影响主线程）\n", errno, EACCES);
    printf("  → errno 是 __thread 变量，每线程一份；不能用它做线程间通信\n\n");

    printf("=== 误用 4：长期保存 strerror 返回的指针 ===\n");
    g_saved_strerror1 = strerror(ENOENT);
    printf("  第一次 strerror(ENOENT) → %p  \"%s\"\n", (void *) g_saved_strerror1,
           g_saved_strerror1);
    g_saved_strerror1 = strerror(EACCES);      /* 下一次调用覆写同一块静态缓冲 */
    printf("  再调 strerror(EACCES)  → %p  \"%s\"\n", (void *) g_saved_strerror1,
           g_saved_strerror1);
    char *again = strerror(ENOENT);
    printf("  第三次 strerror(ENOENT)→ %p  \"%s\"\n", (void *) again, again);
    printf("  → 地址恒定为同一块静态缓冲，**保存指针等于保存「最后一次」的结果**\n");
    printf("    正确：立刻 strcpy/strdup 到自己缓冲；或改用 strerror_r\n\n");

    printf("=== 误用 5：多线程共享 strerror 的静态缓冲 ===\n");
    if (pthread_create(&tid, NULL, worker, NULL) == 0) {
        pthread_join(tid, NULL);
    }
    printf("  线程里算出的指针 %p，主线程拿它的结果 = \"%s\"\n", (void *) g_saved_strerror2,
           g_saved_strerror2);
    printf("  → 两个线程写同一块缓冲，谁后写谁赢 → 数据竞争\n");
    printf("    正确：用 strerror_r / strerror_l，各自传入自己的缓冲\n\n");

    printf("=== 误用 6：把 perror 当万能报错，丢掉上下文 ===\n");
    errno = ENOENT;
    printf("  perror 输出（到 stderr）：");
    fflush(stdout);
    perror("c3_5");
    printf("  perror(msg) 等价于 fprintf(stderr, \"%%s: %%s\\n\", msg, strerror(errno))\n");
    printf("  它只给「哪一步 + 什么错」，给不了「哪个路径、哪个 fd」——业务上下文要自己带\n\n");

    printf("=== 附：glibc 的 %%m 扩展（等价于 strerror(errno)，但不用另起参数）===\n");
    errno = EACCES;
    printf("  printf 里的 %%m 直接展开：errno=%d 就是 \"%m\"\n", errno);
    printf("  ↑ 这是 glibc 私有扩展，可移植代码里别用\n");
    errno = 0;
    return 0;
}
