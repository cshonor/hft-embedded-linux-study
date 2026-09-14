/* TLPI 第 10 章 §10.4 Locales
 * locale 的粒度（LC_ALL vs LC_TIME vs LC_NUMERIC）、这台机器装了哪些 locale、
 * LC_TIME 怎么改 strftime、nl_langinfo / localeconv 取本地化字符串
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_4_locale c10_4_locale.c
 * 运行: ./c10_4_locale                        (无需参数、无需权限)
 *
 * 用固定时间戳，只让 locale 变 → 输出可复现。
 * ⚠️ locale 是**进程级全局状态**，不是线程级的；setlocale 是少数必须
 *    「在程序开头调一次」的函数。
 */
#define _GNU_SOURCE
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FIXED 1700000000L   /* 2023-11-14 22:13:20 UTC */

/* 五个字段的复合格式串。单独拎出来是为了把它**连同展开结果一起打印**——
   下面第 2 节的日志里会先出现这一行原文，读者才知道每行五列各是什么。 */
#define FMT "%A %d %B %Y  |  %c  |  %p  |  %x  |  %X"

static const char *try_list[] = {
    "", "C", "POSIX", "C.UTF-8", "en_US.UTF-8", "en_US.utf8",
    "zh_CN.UTF-8", "zh_CN.utf8", "de_DE.UTF-8", "ja_JP.UTF-8",
    "fr_FR.UTF-8", "lt_LT.UTF-8", NULL
};

static void show_available(void)
{
    printf("== 1. 默认 locale 与这台机器上装了哪些 ==\n");
    printf("setlocale(LC_ALL, NULL) 初始 = \"%s\"\n", setlocale(LC_ALL, NULL));
    printf("（C 程序启动时永远是 \"C\"，除非自己调 setlocale —— "
           "环境变量 LANG/LC_* 不会自动生效）\n\n");

    for (int i = 0; try_list[i] != NULL; i++) {
        char *got;

        setlocale(LC_ALL, "C");                  /* 每次从干净状态试 */
        got = setlocale(LC_ALL, try_list[i]);
        printf("  setlocale(LC_ALL, %-12s) -> %s\n",
               try_list[i][0] ? try_list[i] : "\"\"",
               got ? got : "NULL（这台机器没装）");
        setlocale(LC_ALL, "C");
    }
    printf("\n");
}

static void show_lc_time(void)
{
    struct tm tm;
    time_t t = (time_t) FIXED;
    /* ⚠️ 这个 200 不是随手写的：zh_CN 的 "%A %d %B %Y  |  %c  |  %p  |  %x  |  %X"
       拼起来要 128 字节 —— 原来写 120，strftime 直接返回 0，
       而**返回 0 时缓冲区内容是不确定的**，于是打出 `@??z` 这种垃圾。
       这就是下面第 5 小节要专门演示的坑。 */
    char buf[200];

    printf("== 2. LC_TIME 影响 strftime 的哪些格式符 ==\n");
    printf("  格式串: \"%s\"\n", FMT);
    localtime_r(&t, &tm);

    for (int i = 0; try_list[i] != NULL; i++) {
        char *got;
        size_t n;

        setlocale(LC_ALL, "C");
        got = setlocale(LC_TIME, try_list[i]);
        if (got == NULL)
            continue;                            /* 没装的跳过 */

        n = strftime(buf, sizeof buf, FMT, &tm);
        printf("  LC_TIME=%-12s %s%s\n", try_list[i][0] ? try_list[i] : "\"\"", buf,
               (n == 0) ? "   <-- 返回 0：缓冲区不够！" : "");
    }
    setlocale(LC_ALL, "C");
    printf("\n");
}

/* strftime 唯一的失败信号就是「返回 0」—— 它不会告诉你需要多大，
   而且失败时缓冲区内容**未定义**。这是 C 标准库里少见的「无信息失败」。 */
static void show_ret0_trap(void)
{
    struct tm tm;
    time_t t = (time_t) FIXED;
    char big[512], small[64];

    printf("== 5. 掉一次 strftime 返回 0 的坑 ==\n");
    if (setlocale(LC_ALL, "zh_CN.UTF-8") == NULL) {
        printf("  （这台机器没装 zh_CN.UTF-8，跳过）\n\n");
        setlocale(LC_ALL, "C");
        return;
    }
    localtime_r(&t, &tm);
    {
        size_t need = strftime(big, sizeof big, "%c", &tm);
        size_t n;

        /* 故意先填满，再看 strftime 失败时到底动没动这块内存 */
        memset(small, '#', sizeof small);
        n = strftime(small, 20, "%c", &tm);

        printf("  LC_TIME=zh_CN.UTF-8，格式 \"%%c\"，时间戳 %ld\n", (long) FIXED);
        printf("    完整内容占 %zu 字节（不含结尾 \\0）：\"%s\"\n", need, big);
        printf("    只给 20 字节 -> 返回 %zu  %s\n", n, (n == 0) ? "（0 = 失败）" : "");
        printf("    此时 small = \"%.*s\"\n", 19, small);
        printf("    → 返回 0 时缓冲区内容未定义，**绝不能**再当字符串用；\n");
        printf("      要拿到「需要多大」只能先给个够大的缓冲区试一次。\n\n");
    }
    setlocale(LC_ALL, "C");
}

static void show_langinfo(void)
{
    static const nl_item items[] = {
        CODESET, D_T_FMT, D_FMT, T_FMT, AM_STR, PM_STR,
        RADIXCHAR, THOUSEP, CRNCYSTR
    };
    static const char *names[] = {
        "CODESET", "D_T_FMT", "D_FMT", "T_FMT", "AM_STR", "PM_STR",
        "RADIXCHAR", "THOUSEP", "CRNCYSTR"
    };

    printf("== 3. nl_langinfo()：locale 里的字符串 ==\n");
    for (int pass = 0; pass < 2; pass++) {
        const char *loc = (pass == 0) ? "C" : "de_DE.UTF-8";

        if (setlocale(LC_ALL, loc) == NULL) {
            printf("  （%s 没装，跳过）\n", loc);
            continue;
        }
        printf("  --- locale = %s ---\n", loc);
        for (size_t i = 0; i < sizeof items / sizeof items[0]; i++) {
            const char *v = nl_langinfo(items[i]);
            char e[80];
            size_t j = 0;
            for (size_t k = 0; v[k] != '\0' && j + 2 < sizeof e; k++) {
                if (v[k] == '\n') { e[j++] = '\\'; e[j++] = 'n'; }
                else              { e[j++] = v[k]; }
            }
            e[j] = '\0';
            printf("    %-10s = \"%s\"\n", names[i], e);
        }
    }
    setlocale(LC_ALL, "C");
    printf("\n");
}

static void show_numeric(void)
{
    struct lconv *lc;
    static const int ival = 1234567;
    static const double dval = 1234567.891;

    printf("== 4. LC_NUMERIC 与 printf：小数点是唯一被 printf 认的 ==\n");
    for (int pass = 0; pass < 2; pass++) {
        const char *loc = (pass == 0) ? "C" : "de_DE.UTF-8";

        if (setlocale(LC_ALL, loc) == NULL) {
            printf("  （%s 没装，跳过）\n", loc);
            continue;
        }
        lc = localeconv();
        printf("  --- locale = %s ---\n", loc);
        printf("    localeconv: decimal_point=\"%s\"  thousands_sep=\"%s\"  "
               "currency_symbol=\"%s\"\n",
               lc->decimal_point, lc->thousands_sep, lc->currency_symbol);
        printf("    printf(\"%%f\")      = %f\n", dval);
        printf("    printf(\"%%'d\")     = %'d     <- 千分位要靠 ' 旗标\n", ival);
    }
    setlocale(LC_ALL, "C");
    printf("\n");
}

int main(void)
{
    show_available();
    show_lc_time();
    show_langinfo();
    show_numeric();
    show_ret0_trap();

    printf("四个必须记住的点：\n");
    printf("  1) locale 是**进程级全局**的，C 程序不调 setlocale 永远跑在 \"C\" 上；\n");
    printf("  2) setlocale(LC_ALL, \"\") 才是「按环境变量来」，传 NULL 是「查询」；\n");
    printf("  3) printf 只认 LC_NUMERIC 的小数点；千分位分隔符要显式写 %%'d；\n");
    printf("  4) strftime 失败时**只返回 0**，不给需要的长度，缓冲区内容也不确定\n");
    printf("     —— 这就是 locale=zh_CN 时最容易被咬一口的地方（见第 5 段）。\n");
    return 0;
}
