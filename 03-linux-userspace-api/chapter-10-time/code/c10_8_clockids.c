/* TLPI 第 10 章 §10.8（时钟全景）+ §10.6 补充
 * 把 Linux 上所有 CLOCK_* 摆在一起：数值 / 分辨率 / 当前读数 / 会不会失败
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_8_clockids c10_8_clockids.c
 * 运行: ./c10_8_clockids                        (无需参数、无需权限)
 *
 * 这张表是 §10.6 的收束：同一个 clock_gettime 接口，喂不同的 clockid
 * 拿到的是完全不同的东西 —— 墙上时间 / 单调时间 / CPU 时间 / 粗粒度缓存值。
 * 后面 §10.8 的选型表就是按这张实测表写的。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const struct {
    clockid_t id;
    const char *name;
    const char *kind;
} ids[] = {
    { CLOCK_REALTIME,             "CLOCK_REALTIME",             "墙上时间" },
    { CLOCK_MONOTONIC,            "CLOCK_MONOTONIC",            "单调" },
    { CLOCK_MONOTONIC_RAW,        "CLOCK_MONOTONIC_RAW",        "单调(不校正)" },
    { CLOCK_BOOTTIME,             "CLOCK_BOOTTIME",             "单调(含挂起)" },
    { CLOCK_REALTIME_COARSE,      "CLOCK_REALTIME_COARSE",      "粗粒度" },
    { CLOCK_MONOTONIC_COARSE,     "CLOCK_MONOTONIC_COARSE",     "粗粒度" },
    { CLOCK_PROCESS_CPUTIME_ID,   "CLOCK_PROCESS_CPUTIME_ID",   "进程 CPU" },
    { CLOCK_THREAD_CPUTIME_ID,    "CLOCK_THREAD_CPUTIME_ID",    "线程 CPU" },
    { CLOCK_REALTIME_ALARM,       "CLOCK_REALTIME_ALARM",       "闹钟" },
    { CLOCK_BOOTTIME_ALARM,       "CLOCK_BOOTTIME_ALARM",       "闹钟" },
    { CLOCK_TAI,                  "CLOCK_TAI",                  "国际原子时" },
};

static void one(clockid_t id, const char *name, const char *kind)
{
    struct timespec res, now;
    int rr, nr;

    errno = 0;
    rr = clock_getres(id, &res);
    errno = 0;
    nr = clock_gettime(id, &now);

    printf("  %-24s id=0x%08x  %-12s ", name, (unsigned) id, kind);

    if (rr == 0)
        printf("res=%9ld ns  ", (long) res.tv_sec * 1000000000L + res.tv_nsec);
    else
        printf("res=%-12s ", "ERR");

    if (nr == 0)
        printf("now=%12ld.%09ld\n", (long) now.tv_sec, (long) now.tv_nsec);
    else
        printf("now 失败 errno=%d (%s)\n", errno, strerror(errno));
}

int main(void)
{
    struct timespec rt, mono, raw, boot, tai;

    printf("== 1. 所有 CLOCK_* 逐个试 ==\n");
    for (size_t i = 0; i < sizeof ids / sizeof ids[0]; i++)
        one(ids[i].id, ids[i].name, ids[i].kind);
    printf("\n");

    printf("== 2. 把几个关键时钟放在一起比 ==\n");
    clock_gettime(CLOCK_REALTIME, &rt);
    clock_gettime(CLOCK_MONOTONIC, &mono);
    clock_gettime(CLOCK_MONOTONIC_RAW, &raw);
    clock_gettime(CLOCK_BOOTTIME, &boot);
    clock_gettime(CLOCK_TAI, &tai);

    printf("  REALTIME     = %ld.%09ld   Epoch 起算（可被 NTP 改）\n",
           (long) rt.tv_sec, (long) rt.tv_nsec);
    printf("  MONOTONIC    = %ld.%09ld   开机起算，被 NTP 用 slewing 慢慢拉\n",
           (long) mono.tv_sec, (long) mono.tv_nsec);
    printf("  MONOTONIC_RAW= %ld.%09ld   硬件时间源直接换算，NTP 完全碰不到\n",
           (long) raw.tv_sec, (long) raw.tv_nsec);
    printf("  BOOTTIME     = %ld.%09ld   比 MONOTONIC 多算挂起时间\n",
           (long) boot.tv_sec, (long) boot.tv_nsec);
    printf("  TAI          = %ld.%09ld   原子时\n", (long) tai.tv_sec, (long) tai.tv_nsec);
    printf("\n");
    printf("  RAW - MONO         = %+ld ns    ← NTP 累计 slew 掉的量\n",
           (long) ((raw.tv_sec - mono.tv_sec) * 1000000000L + (raw.tv_nsec - mono.tv_nsec)));
    printf("  BOOTTIME - MONO    = %+ld ns    ← 容器里没挂起过，所以是 0\n",
           (long) ((boot.tv_sec - mono.tv_sec) * 1000000000L + (boot.tv_nsec - mono.tv_nsec)));
    printf("  TAI - REALTIME     = %+ld ns    ← 闰秒补偿；内核没加载闰秒表时是 0\n",
           (long) ((tai.tv_sec - rt.tv_sec) * 1000000000L + (tai.tv_nsec - rt.tv_nsec)));
    printf("  REALTIME - MONO    = %+ld ns    ← 两个起点不同，这个差值没有意义\n",
           (long) ((rt.tv_sec - mono.tv_sec) * 1000000000L + (rt.tv_nsec - mono.tv_nsec)));
    printf("     （但它的**变化率**有意义：如果它突然变大/变小，说明 REALTIME 被调了）\n\n");

    printf("== 3. 两个 ALARM 时钟：读得到，但「定定时器」要额外能力 ==\n");
    {
        struct timespec ts;

        errno = 0;
        if (clock_gettime(CLOCK_REALTIME_ALARM, &ts) == 0)
            printf("  clock_gettime(CLOCK_REALTIME_ALARM) 成功 —— 读不需要特权\n");
        else
            printf("  clock_gettime(CLOCK_REALTIME_ALARM) 失败 errno=%d (%s)\n",
                   errno, strerror(errno));
        printf("  但用 timer_create() 挂到 ALARM 时钟上就需要 CAP_WAKE_ALARM，\n");
        printf("  目的只有一个：让系统从 suspend 里被唤醒（Ch23 会实测）。\n");
    }
    printf("\n");

    printf("== 4. 选型口径（§10.8 那张表的实测依据） ==\n");
    printf("  要人可读日期时间        -> CLOCK_REALTIME\n");
    printf("  要测间隔/延迟/超时      -> CLOCK_MONOTONIC（绝不用 REALTIME）\n");
    printf("  要基准测试、排除 NTP    -> CLOCK_MONOTONIC_RAW\n");
    printf("  高频打时间戳、容忍粗粒度-> CLOCK_MONOTONIC_COARSE（最便宜）\n");
    printf("  要算「这函数花了多少 CPU」-> CLOCK_PROCESS_CPUTIME_ID / THREAD\n");
    printf("  要含休眠时长            -> CLOCK_BOOTTIME\n");
    return 0;
}
