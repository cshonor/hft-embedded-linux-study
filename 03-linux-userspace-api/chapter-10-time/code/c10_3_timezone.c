/* TLPI 第 10 章 §10.3 Timezones
 * 时区怎么被定下来：tzset() / tzname[] / 全局 timezone 与 daylight /
 * tm_gmtoff / TZ 环境变量的四种写法 / POSIX TZ 的符号陷阱 / DST 切换
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_3_timezone c10_3_timezone.c
 * 运行: ./c10_3_timezone                     (无需参数、无需权限)
 *
 * 全部用**固定时间戳**，只有 TZ 是我们主动改的变量 → 输出可复现。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SUMMER 1688169600L   /* 2023-07-01 00:00:00 UTC */
#define WINTER 1701388800L   /* 2023-12-01 00:00:00 UTC */

static void show_localtime_file(void)
{
    char link[256];
    ssize_t n = readlink("/etc/localtime", link, sizeof link - 1);

    printf("== 1. 系统时区是从哪儿读来的 ==\n");
    if (n > 0) {
        link[n] = '\0';
        printf("/etc/localtime -> %s\n", link);
    } else {
        printf("/etc/localtime 读不到（readlink 返回 %zd）\n", n);
    }
    /* 不带前导冒号的 TZ 值 glibc 会去 /usr/share/zoneinfo/ 下查这个文件 */
    printf("/usr/share/zoneinfo/Asia/Shanghai 是否可读: %s\n",
           (access("/usr/share/zoneinfo/Asia/Shanghai", R_OK) == 0) ? "是" : "否");
    printf("TZ 环境变量 = %s\n\n",
           getenv("TZ") ? getenv("TZ") : "(未设置)");
}

/* 用两个独立途径算同一个偏移，互相印证 */
static void show_offset_two_ways(time_t t, const char *label)
{
    struct tm l;
    long via_gmtoff, via_diff;

    localtime_r(&t, &l);
    via_gmtoff = l.tm_gmtoff;
    /* 把本地分解时间当成 UTC 再解释一次，差值就是偏移 */
    via_diff = (long) mktime(&l) - (long) timegm(&l);

    /* ⚠️ 这里刻意把「一致?」写成「== -tm_gmtoff?」：
       mktime() 把字段当**本地时间**解释，timegm() 当 **UTC** 解释，
       两者之差 = -偏移。所以正确的互证条件是 via_diff == -via_gmtoff，
       而不是相等 —— 第一次写成「相等」时这四行全是「否」，看着像 bug。 */
    printf("  %-6s %s  %-3s  %+03ld%02ld   tm_gmtoff=%+ld s  "
           "mktime-timegm=%+ld s  互为相反数? %s\n",
           label, l.tm_zone, tzname[l.tm_isdst > 0 ? 1 : 0],
           via_gmtoff / 3600, labs(via_gmtoff % 3600) / 60,
           via_gmtoff, via_diff, (via_diff == -via_gmtoff) ? "是" : "否");
}

static void show_globals(void)
{
    printf("== 2. tzset() 与三个全局变量 ==\n");
    tzset();
    printf("tzname[0] = \"%s\"   tzname[1] = \"%s\"\n", tzname[0], tzname[1]);
    printf("全局 timezone = %ld s（UTC 以西为正，注意符号！）\n", timezone);
    printf("全局 daylight = %d\n", daylight);
    printf("（这三个都是 glibc 的全局变量，靠 tzset() 刷新 —— "
           "别人改了 TZ 你这里不会自动变）\n\n");
}

static void show_tz_switch(void)
{
    static const char *tzs[] = {
        "UTC",
        "Asia/Shanghai",
        "America/New_York",
        "Europe/London",
        ":Asia/Tokyo",
        "XXX-8",
        "UTC+8",
        "IST-5:30",
        "EST5EDT,M3.2.0,M11.1.0",
        NULL
    };

    printf("== 3. 进程内切换 TZ：setenv + tzset ==\n");
    for (int i = 0; tzs[i] != NULL; i++) {
        struct tm l;
        char buf[80];
        time_t wt = (time_t) WINTER;

        setenv("TZ", tzs[i], 1);
        tzset();
        printf("TZ=%-26s tzname[0]=%-12s timezone=%6ld  daylight=%d\n",
               tzs[i], tzname[0], timezone, daylight);
        localtime_r(&wt, &l);
        strftime(buf, sizeof buf, "%F %T %Z %z", &l);
        printf("%-34s -> %s\n", "", buf);
    }
    printf("\n");
}

static void show_dst(void)
{
    struct tm s, w;
    char bs[80], bw[80];
    time_t st = (time_t) SUMMER, wt = (time_t) WINTER;

    printf("== 4. DST：同一时区、两个季节 ==\n");
    setenv("TZ", "America/New_York", 1);
    tzset();
    localtime_r(&st, &s);
    localtime_r(&wt, &w);
    strftime(bs, sizeof bs, "%F %T %Z %z", &s);
    strftime(bw, sizeof bw, "%F %T %Z %z", &w);
    printf("2023-07-01 -> %s   tm_isdst=%d\n", bs, s.tm_isdst);
    printf("2023-12-01 -> %s   tm_isdst=%d\n", bw, w.tm_isdst);
    printf("偏移差了 %ld 分钟\n\n", (w.tm_gmtoff - s.tm_gmtoff) / 60);

    printf("== 5. 两个季节在两个时区上的偏移 ==\n");
    setenv("TZ", "Asia/Shanghai", 1);
    tzset();
    show_offset_two_ways((time_t) SUMMER, "summer");
    show_offset_two_ways((time_t) WINTER, "winter");
    setenv("TZ", "America/New_York", 1);
    tzset();
    show_offset_two_ways((time_t) SUMMER, "summer");
    show_offset_two_ways((time_t) WINTER, "winter");
    printf("\n");
}

int main(void)
{
    show_localtime_file();
    show_globals();
    show_tz_switch();
    show_dst();

    printf("⚠️ 两个必须记住的陷阱：\n");
    printf("  1) POSIX TZ 的符号是**反的**。TZ=XXX-8 表示「比 UTC 早 8 小时」\n");
    printf("     （即东八区，offset = +0800）；所以 TZ=UTC+8 得到的是 -08。\n");
    printf("  2) 不带前导冒号的 TZ 值会被 glibc 先去 /usr/share/zoneinfo 找同名\n");
    printf("     文件；加冒号（TZ=:Asia/Tokyo）是强制走「文件路径」语义。\n");
    return 0;
}
