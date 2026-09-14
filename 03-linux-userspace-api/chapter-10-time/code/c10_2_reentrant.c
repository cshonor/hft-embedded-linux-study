/* TLPI 第 10 章 §10.2 Time-Conversion Functions（可重入性）
 * gmtime()/localtime()/asctime()/ctime() 返回的都是**静态缓冲区**，
 * 两次调用会互相覆盖 —— 现场抓地址 + 现场看数据被改掉。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_2_reentrant c10_2_reentrant.c
 * 运行: ./c10_2_reentrant                    (无需参数、无需权限)
 *
 * 输出里的地址每次运行不同（ASLR + 库加载基址），要看的是：
 *   * 两次调用返回的**指针是否相同**；
 *   * 第一个结果里的数据是不是**被第二次调用改掉了**。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>

#define T1 1700000000L   /* 2023-11-14 22:13:20 UTC */
#define T2 0L            /* 1970-01-01 00:00:00 UTC */

static void show_tm_clobber(void)
{
    time_t a = (time_t) T1, b = (time_t) T2;
    struct tm *p1, *p2;

    printf("== 1. gmtime() / localtime() 共用同一个静态 struct tm ==\n");
    p1 = gmtime(&a);
    p2 = gmtime(&b);
    printf("p1 = gmtime(&%ld) -> %p\n", (long) a, (void *) p1);
    printf("p2 = gmtime(&%ld) -> %p\n", (long) b, (void *) p2);
    printf("两个指针相同? %s\n", (p1 == p2) ? "是 —— 同一个缓冲区" : "否");
    printf("通过 p1 读出的年份 = %d   <- 期望 %d，实际是 %d\n",
           p1->tm_year + 1900, 1900 + 123, p1->tm_year + 1900);
    printf("（p1 看到的是第二次调用的结果，第一次的结果已经没了）\n\n");

    p1 = gmtime(&a);
    p2 = localtime(&a);
    printf("gmtime() 与 localtime() 也是同一个缓冲区? %s\n\n",
           (p1 == p2) ? "是" : "否");
}

static void show_tm_reentrant(void)
{
    time_t a = (time_t) T1, b = (time_t) T2;
    struct tm ta, tb;
    struct tm *r1, *r2;

    printf("== 2. _r 版本：调用者自己给缓冲区 ==\n");
    r1 = gmtime_r(&a, &ta);
    r2 = gmtime_r(&b, &tb);
    printf("gmtime_r(&%ld) -> %p   ta.tm_year=%d\n", (long) a, (void *) r1, ta.tm_year + 1900);
    printf("gmtime_r(&%ld) -> %p   tb.tm_year=%d\n", (long) b, (void *) r2, tb.tm_year + 1900);
    printf("两个缓冲区相同? %s\n", (r1 == r2) ? "是" : "否 —— 各自独立");
    printf("ta 里还是第一次的结果? %s\n\n",
           (ta.tm_year + 1900 == 2023 && tb.tm_year + 1900 == 1970) ? "是" : "否");
}

static void show_str_clobber(void)
{
    time_t a = (time_t) T1, b = (time_t) T2;
    struct tm ga, gb;
    char *s1, *s2;
    char ba[64], bb[64];

    printf("== 3. asctime() 共用同一个静态字符数组 ==\n");
    gmtime_r(&a, &ga);
    gmtime_r(&b, &gb);

    s1 = asctime(&ga);
    printf("第 1 次 asctime(&ga) -> %p  内容: %s", (void *) s1, s1);
    s2 = asctime(&gb);
    printf("第 2 次 asctime(&gb) -> %p  内容: %s", (void *) s2, s2);
    printf("两个指针相同? %s\n", (s1 == s2) ? "是 —— 同一个缓冲区" : "否");
    printf("现在拿第 1 次的指针 s1 再读一次: %s", s1);
    printf("  ^^ 第 1 次的结果已被第 2 次覆盖\n\n");

    printf("== 4. ctime() 同样共用缓冲区（它就是 localtime + asctime） ==\n");
    s1 = ctime(&a);
    printf("第 1 次 ctime(&%ld) -> %p  内容: %s", (long) a, (void *) s1, s1);
    s2 = ctime(&b);
    printf("第 2 次 ctime(&%ld) -> %p  内容: %s", (long) b, (void *) s2, s2);
    printf("两个指针相同? %s\n\n", (s1 == s2) ? "是" : "否");

    printf("== 5. _r 版本：调用者自己给缓冲区 ==\n");
    printf("asctime_r(&ga, ba) -> %s", asctime_r(&ga, ba));
    printf("asctime_r(&gb, bb) -> %s", asctime_r(&gb, bb));
    printf("两个缓冲区互不影响：ba=\"%s\"  bb=\"%s\"\n",
           ba, bb);
    ctime_r(&a, ba);
    printf("ctime_r(&%ld, ba)  -> %s", (long) a, ba);
    printf("此时 bb 仍是: %s", bb);
}

int main(void)
{
    show_tm_clobber();
    show_tm_reentrant();
    show_str_clobber();
    printf("\n结论：主线程里单次用没问题；一旦要同时持有两个分解时间，"
           "或者多线程/信号处理器里也要转时间，就必须用 _r 版本。\n");
    return 0;
}
