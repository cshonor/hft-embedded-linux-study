/* TLPI 第 10 章 §10.2 Time-Conversion Functions
 * strftime() 格式符全表 + struct tm 字段语义 + 往返回
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_2_strftime c10_2_strftime.c
 * 运行: ./c10_2_strftime                    (无需参数、无需权限)
 *
 * ⚠️ 本 demo 故意用**固定时间戳** 1700000000（= 2023-11-14 22:13:20 UTC），
 *    不用 time(NULL)。这样笔记里的实测输出才能逐字复现、不随运行时刻漂移。
 *    （这是本仓库写「会被抄进笔记的输出」的统一约定。）
 *
 * 三个必须现场看到的点：
 *   1. struct tm 的 tm_year 是「年 - 1900」、tm_mon 是 0..11 —— 字符串里看到的
 *      年份不是字段里的数值；
 *   2. 同一时间戳喂给 gmtime() 与 localtime()，得到两个 struct tm；
 *   3. %s 不是 C 标准（C99 没有它），是 POSIX/glibc 扩展 —— 用 gcc -std=c99
 *      加 -pedantic 会告警，但这里不告警是因为默认是 gnu 模式。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIXED_EPOCH 1700000000L

/* 把不可见字符转义后再打印，否则 %n / %t 会把输出表格冲乱 */
static void esc(const char *s, char *out, size_t n)
{
    size_t j = 0;

    for (size_t i = 0; s[i] != '\0' && j + 2 < n; i++) {
        if (s[i] == '\n')      { out[j++] = '\\'; out[j++] = 'n'; }
        else if (s[i] == '\t') { out[j++] = '\\'; out[j++] = 't'; }
        else                   { out[j++] = s[i]; }
    }
    out[j] = '\0';
}

static void show_struct_tm(void)
{
    time_t t = (time_t) FIXED_EPOCH;
    struct tm g;

    gmtime_r(&t, &g);
    printf("== 1. struct tm 的字段就是内核/库给你的原始数值 ==\n");
    printf("tm_sec   = %2d      tm_min  = %2d      tm_hour = %2d\n",
           g.tm_sec, g.tm_min, g.tm_hour);
    printf("tm_mday  = %2d      tm_mon  = %2d   <- 0..11，不是 1..12\n",
           g.tm_mday, g.tm_mon);
    printf("tm_year  = %2d   <- 年 - 1900，所以真实年份 = %d\n",
           g.tm_year, g.tm_year + 1900);
    printf("tm_wday  = %2d   <- 0=Sunday\n", g.tm_wday);
    printf("tm_yday  = %2d   <- 0..365，不是 1..366\n", g.tm_yday);
    printf("tm_isdst = %2d   <- gmtime 恒为 0\n\n", g.tm_isdst);
}

static void show_formats(void)
{
    static const char *fmts[] = {
        "%a", "%A", "%b", "%B", "%c", "%C", "%d", "%D", "%e", "%F",
        "%g", "%G", "%h", "%H", "%I", "%j", "%m", "%M", "%n", "%p",
        "%r", "%R", "%s", "%S", "%t", "%T", "%u", "%U", "%V", "%w",
        "%W", "%x", "%X", "%y", "%Y", "%z", "%Z", "%%", NULL
    };
    time_t t = (time_t) FIXED_EPOCH;
    struct tm g;

    gmtime_r(&t, &g);
    printf("== 2. strftime 格式符全表（固定时间戳 1700000000，用 gmtime） ==\n");
    for (int i = 0; fmts[i] != NULL; i++) {
        char buf[200], e[210];
        size_t n = strftime(buf, sizeof buf, fmts[i], &g);
        esc(buf, e, sizeof e);
        printf("  %-4s -> \"%s\"%s\n", fmts[i], e, (n == 0) ? "   <-- 返回 0！" : "");
    }
    printf("\n");
}

static void show_gm_vs_local(void)
{
    time_t t = (time_t) FIXED_EPOCH;
    struct tm g, l;
    char bg[80], bl[80];

    gmtime_r(&t, &g);
    localtime_r(&t, &l);
    /* %Z 在 gmtime 下由 glibc 填 "GMT"；localtime 下取 tzname[] */
    strftime(bg, sizeof bg, "%F %T %Z (%z)", &g);
    strftime(bl, sizeof bl, "%F %T %Z (%z)", &l);

    printf("== 3. 同一时间戳，两条分解路径 ==\n");
    printf("gmtime()    -> %s\n", bg);
    printf("localtime() -> %s\n", bl);
    printf("容器的 TZ     = %s\n",
           getenv("TZ") ? getenv("TZ") : "(未设置，glibc 回退读 /etc/localtime)");
    printf("两者相同?     %s   <- 容器默认 UTC 时二者必然相同\n\n",
           (l.tm_hour == g.tm_hour && l.tm_min == g.tm_min) ? "是" : "否（容器非 UTC）");
}

static void show_round_trip(void)
{
    /* strftime 出去，strptime 回来，mktime 归一 —— 原书 Listing 10-3 的主线 */
    const char *in = "2023-11-14 22:13:20";
    const char *infmt = "%Y-%m-%d %H:%M:%S";
    struct tm tm;
    char out[80];

    memset(&tm, 0, sizeof tm);
    if (strptime(in, infmt, &tm) == NULL) {
        printf("strptime 失败\n");
        return;
    }
    tm.tm_isdst = -1;   /* strptime 不设这个字段，交给 mktime 自己判定 */

    printf("== 4. strftime / strptime / mktime 往返回 ==\n");
    printf("输入字符串      = \"%s\"   格式 %s\n", in, infmt);
    printf("strptime 后 tm  = year=%d mon=%d mday=%d %02d:%02d:%02d\n",
           tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("mktime 还原的秒 = %ld   <- strptime 之后 tm_isdst 仍是 %d\n",
           (long) mktime(&tm), tm.tm_isdst);
    strftime(out, sizeof out, "%A, %d %B %Y %H:%M:%S %Z", &tm);
    printf("再 strftime     = %s\n", out);
    printf("和原时间戳相等? %s\n", (mktime(&tm) == (time_t) FIXED_EPOCH) ? "是" : "否");
}

int main(void)
{
    show_struct_tm();
    show_formats();
    show_gm_vs_local();
    show_round_trip();
    return 0;
}
