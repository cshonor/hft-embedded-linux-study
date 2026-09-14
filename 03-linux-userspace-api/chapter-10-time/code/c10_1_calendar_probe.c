/* TLPI 第 10 章 §10.1 Calendar Time
 * 日历时间的三种读法 / Epoch 原点 / time_t 的真实宽度与 2038 边界
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_1_calendar_probe c10_1_calendar_probe.c
 * 运行: ./c10_1_calendar_probe            (无需参数、无需权限)
 *
 * 要钉住的四件事：
 *   1. time() / gettimeofday() / clock_gettime(CLOCK_REALTIME) 读的是**同一个**
 *      墙上时钟，只是分辨率与容器类型不同；
 *   2. time() 的分辨率**恰好是 1 秒**——它就是把精确时钟的 tv_sec 截下来；
 *   3. Epoch 原点是 1970-01-01 00:00:00 **UTC**，但 ctime() 按**本地时区**渲染；
 *   4. 2038 问题在这台机器上**不存在**（time_t 已是 64 位），把 INT32_MAX
 *      当时间戳喂进去才能看到那个著名的边界。
 */
#define _GNU_SOURCE
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

/* 照抄原书 Listing 10-1 的常量：一个回归年的秒数（用 365.24219 天） */
#define SECONDS_IN_TROPICAL_YEAR (365.24219 * 24 * 60 * 60)

static void show_three_reads(void)
{
    time_t t = time(NULL);
    struct timeval tv;
    struct timespec ts;

    gettimeofday(&tv, NULL);
    clock_gettime(CLOCK_REALTIME, &ts);

    printf("== 1. 同一个墙上时钟的三种读法 ==\n");
    printf("time()                  = %ld\n", (long) t);
    printf("gettimeofday()          = %ld s + %ld us  (tv_usec 上限 999999)\n",
           (long) tv.tv_sec, (long) tv.tv_usec);
    printf("clock_gettime(REALTIME) = %ld s + %ld ns  (tv_nsec 上限 999999999)\n",
           (long) ts.tv_sec, (long) ts.tv_nsec);
    printf("三者 tv_sec 同秒?         %s\n",
           (t == tv.tv_sec && tv.tv_sec == ts.tv_sec) ? "是" : "否（恰好跨秒）");
    printf("time() 丢掉的小数部分    = 0.%09ld s\n\n", (long) ts.tv_nsec);
}

static void show_epoch(void)
{
    time_t zero = 0;
    time_t neg = -1;
    struct tm z;
    char buf[64];

    printf("== 2. Epoch 原点与「负数时间」 ==\n");
    /* ctime() 的日字段是右对齐两位，1 号前面有两个空格 */
    printf("time_t  0 -> ctime()  = %s", ctime(&zero));
    gmtime_r(&zero, &z);
    strftime(buf, sizeof buf, "%F %T", &z);
    printf("time_t  0 -> UTC 格式 = %s\n", buf);
    /* -1 落在 Epoch 前一秒：1969-12-31 23:59:59 */
    printf("time_t -1 -> ctime()  = %s", ctime(&neg));
    printf("\n");
}

static void show_width(void)
{
    time_t t32 = (time_t) INT32_MAX;
    struct tm tm32;
    char buf[64];

    printf("== 3. time_t 的真实宽度与 2038 边界 ==\n");
    printf("sizeof(time_t) = %zu 字节   sizeof(long) = %zu 字节\n",
           sizeof(time_t), sizeof(long));
    printf("sizeof(time_t) == sizeof(long)? %s\n",
           sizeof(time_t) == sizeof(long) ? "是" : "否");
    if (sizeof(time_t) == 8) {
        printf("time_t 是 64 位 -> 本机**没有** 2038 问题（上界约 2924 亿年后）\n");
    } else {
        printf("time_t 是 32 位 -> 本机**有** 2038 问题\n");
    }
    if (gmtime_r(&t32, &tm32) != NULL) {
        strftime(buf, sizeof buf, "%F %T", &tm32);
        printf("把 INT32_MAX 当时间戳（即 32 位 time_t 的上界）:\n");
        printf("  %ld = 0x%08lx -> %s UTC\n",
               (long) t32, (unsigned long) t32 & 0xffffffffUL, buf);
    }
    printf("  同一个值按本地时区看      -> %s", ctime(&t32));
    printf("  这个值就是 2038 问题的引爆点\n\n");
}

int main(void)
{
    show_three_reads();
    show_epoch();
    show_width();

    {
        time_t t = time(NULL);
        printf("== 4. Epoch 起算的绝对时间（原书 Listing 10-1 的开场计算） ==\n");
        printf("seconds since the Epoch = %ld\n", (long) t);
        printf("  about %6.3f years\n", t / SECONDS_IN_TROPICAL_YEAR);
    }
    return 0;
}
