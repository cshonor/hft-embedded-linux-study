/* TLPI 第 10 章 §10.3/§10.8 配套自编练习（非原书习题）
 * 正确地把「纳秒时间戳」打成带时区偏移的 ISO 8601 字符串
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex10_2_utc_offset ex10_2_utc_offset.c
 * 运行: ./ex10_2_utc_offset                     (无需参数、无需权限)
 *
 * 这道题针对的是一个非常常见的写法错误：把时区偏移**硬编码**成 "+0800"。
 * 正确做法是从 struct tm 的 tm_gmtoff 字段（glibc/BSD 扩展）算出来，
 * 它自动带正负号、也自动处理半小时/三刻钟时区（如 +05:30、+05:45）。
 *
 * 全部用固定时间戳，只有 TZ 在变 → 输出可复现。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIXED 1700000000L   /* 2023-11-14 22:13:20 UTC */

static const char *tzs[] = {
    "UTC", "Asia/Shanghai", "America/New_York", "Asia/Kolkata",
    "Asia/Kathmandu", "Australia/Adelaide", NULL
};

/* 错误写法：偏移写死 */
static void wrong_way(time_t t, char *out, size_t n)
{
    struct tm tm;
    char base[64];

    localtime_r(&t, &tm);
    strftime(base, sizeof base, "%Y-%m-%d %H:%M:%S", &tm);
    /* 硬编码「中国是 +8」，换任何别的时区都错 */
    snprintf(out, n, "%s +0800", base);
}

/* 正确写法：偏移从 tm_gmtoff 算 */
static void right_way(const struct timespec *ts, const struct tm *tm,
                      char *out, size_t n)
{
    char base[64];
    long off = tm->tm_gmtoff;
    char sign = (off < 0) ? '-' : '+';
    long ao = (off < 0) ? -off : off;

    strftime(base, sizeof base, "%Y-%m-%dT%H:%M:%S", tm);
    snprintf(out, n, "%s.%06ld%c%02ld:%02ld",
             base, (long) (ts->tv_nsec / 1000), sign, ao / 3600, (ao % 3600) / 60);
}

int main(void)
{
    struct timespec ts = { FIXED, 123456789L };   /* 故意给个非零纳秒 */
    char w[80], r[80];

    printf("== 1. 错误写法 vs 正确写法（同一个时间戳，逐个时区看） ==\n");
    printf("  固定时间戳 %ld，纳秒 123456789 -> 微秒 %ld\n\n",
           (long) ts.tv_sec, (long) (ts.tv_nsec / 1000));

    for (int i = 0; tzs[i] != NULL; i++) {
        struct tm tm;
        char z1[80], z2[80];

        setenv("TZ", tzs[i], 1);
        tzset();
        localtime_r(&ts.tv_sec, &tm);

        wrong_way(ts.tv_sec, w, sizeof w);
        right_way(&ts, &tm, r, sizeof r);
        strftime(z1, sizeof z1, "%Z", &tm);     /* 缩写，没有冒号 */
        strftime(z2, sizeof z2, "%z", &tm);     /* +0800，没有冒号 */

        printf("  TZ=%-22s %%-Z=%-5s %%-z=%-6s\n", tzs[i], z1, z2);
        printf("      错误: %s\n", w);
        printf("      正确: %s\n", r);
    }
    printf("\n");

    printf("== 2. 想要「带冒号」的偏移？%%:z 这条路走不通 ==\n");
    {
        /* ⚠️ 这段要**用变量格式串**才能观察到两件事：
             ① gcc 的 strftime 格式检查器（-Wformat=）不认 %:z，写字面量直接告警；
             ② 实测 glibc 2.39 **根本不实现** %:z / %::z / %:::z ——
                strftime 把它当成「无法识别的转换」，原样吐字面量
                （返回值 = 字面量长度 3，输出就是那三个字符）。
           所以网上说的「glibc 支持 %:z」在本机是**假的**：要冒号只能自己拼。 */
        static const char *colon_fmt = "%:z";

        for (int i = 0; tzs[i] != NULL; i++) {
            struct tm tm;
            char b1[80], b2[80], mine[80];
            long off;
            size_t n;

            setenv("TZ", tzs[i], 1);
            tzset();
            localtime_r(&ts.tv_sec, &tm);

            strftime(b1, sizeof b1, "%z", &tm);
            n = strftime(b2, sizeof b2, colon_fmt, &tm);

            /* 自己拼带冒号的版本：偏移从 tm_gmtoff 算，符号与分种数都自动对 */
            off = tm.tm_gmtoff;
            snprintf(mine, sizeof mine, "%c%02ld:%02ld",
                     off < 0 ? '-' : '+', labs(off) / 3600, labs(off % 3600) / 60);

            printf("  TZ=%-22s %%-z=%-7s %%-:z=\"%s\"(ret=%zu)  自己拼=%s\n",
                   tzs[i], b1, b2, n, mine);
        }
    }
    printf("\n");

    printf("== 3. 半/三刻钟时区是最容易写错的地方 ==\n");
    for (int i = 0; tzs[i] != NULL; i++) {
        struct tm tm;
        long off;

        setenv("TZ", tzs[i], 1);
        tzset();
        localtime_r(&ts.tv_sec, &tm);
        off = tm.tm_gmtoff;
        if (off % 3600 == 0)
            continue;
        printf("  %-22s tm_gmtoff = %+ld s -> %+03ld:%02ld\n",
               tzs[i], off, off / 3600, labs(off % 3600) / 60);
    }
    printf("\n");

    printf("== 4. 结论 ==\n");
    printf("  * 时区偏移必须从 tm_gmtoff 取（或从 %%z 取），**不要硬编码**；\n");
    printf("  * 亚秒部分要自己打：strftime 的 %%S 只到秒，纳秒/微秒得单独拼；\n");
    printf("  * 纳秒 -> 微秒是**截断**除法（/1000），不是四舍五入；\n");
    printf("  * ISO 8601 的偏移推荐带冒号（+08:00），%%z 给的是无冒号形式（+0800）；\n");
    printf("    glibc 2.39 的 strftime **不认** %%:z（原样吐字面量，见第 2 段实测），\n");
    printf("    要冒号只能像 right_way() 那样从 tm_gmtoff 自己拼。\n");
    return 0;
}
