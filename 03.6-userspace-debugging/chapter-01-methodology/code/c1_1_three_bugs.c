/*
 * c1_1_three_bugs.c —— 三类典型 bug 的「症状样板」
 *
 * 用途：Ch1 的方法论需要一份「拿在手里的症状」。同一个程序，三颗雷，
 *       分别对应「崩溃 / 内存 / 并发」三类问题，是 1.1 分类学与
 *       1.2 症状→工具决策树的实体道具。
 *
 * 用法：./c1_1_three_bugs <case>
 *         case 1 = 崩溃：空指针解引用（稳定复现）
 *         case 2 = 内存：只借不还（泄漏）
 *         case 3 = 并发：两个线程写同一个全局计数（数据竞争）
 *
 * 编译（默认，不加任何 sanitizer）：
 *   gcc -g -O0 -Wall -Wextra -o c1_1_three_bugs c1_1_three_bugs.c -pthread
 * 编译（并发那颗雷要看得见，必须用 clang 的 TSan）：
 *   clang -g -O1 -fsanitize=thread -pthread -o c1_1_three_bugs_tsan c1_1_three_bugs.c
 * 编译（内存那颗雷要看得见，用 ASan/LSan）：
 *   gcc -g -O1 -fsanitize=address -o c1_1_three_bugs_asan c1_1_three_bugs.c -pthread
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- case 3 用：非原子的共享计数 ---- */
static long g_orders_matched = 0;

static void *match_worker(void *arg)
{
    long n = (long)arg;
    for (long i = 0; i < n; i++)
        g_orders_matched++;                 /* 非原子读改写：数据竞争 */
    return NULL;
}

/* ---- case 1：空指针解引用 ---- */
static int crash_null_deref(void)
{
    struct order {
        long id;
        long qty;
    };
    struct order *o = NULL;

    printf("case 1: 准备解引用一个 NULL 订单指针\n");
    fflush(stdout);
    o->qty = 100;                            /* ← SIGSEGV 在这里 */
    return 0;
}

/* ---- case 2：只借不还 ---- */
static int leak_orders(void)
{
    for (int i = 0; i < 100; i++) {
        char *fill = malloc(400);            /* 400 B × 100 = 40000 B，从未 free */
        if (!fill)
            return 1;
        memset(fill, 0, 400);
    }
    printf("case 2: 100 个 400 字节的订单快照已分配（全部未释放）\n");
    return 0;
}

/* ---- case 3：两个线程抢同一个计数器 ---- */
static int race_counter(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, match_worker, (void *)100000L);
    pthread_create(&t2, NULL, match_worker, (void *)100000L);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("case 3: 期望撮合 200000 笔，实际记到 %ld 笔\n", g_orders_matched);
    return 0;
}

int main(int argc, char **argv)
{
    int which = (argc > 1) ? atoi(argv[1]) : 0;

    if (which == 1)
        return crash_null_deref();
    if (which == 2)
        return leak_orders();
    if (which == 3)
        return race_counter();

    fprintf(stderr, "用法: %s <1|2|3>\n", argv[0]);
    return 2;
}
