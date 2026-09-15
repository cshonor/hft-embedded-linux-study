/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch04 程序」（copy.c / tee 等）*
*  能零改动编译而存在。原书的头文件散在 Listing 3-1 + 3-2 + 3-5，本仓库  *
*  没有随附，见 Ch03 笔记。                                               *
*                                                                         *
*  ⚠️ 与 Ch10 的同名替身**不通用**：那一份的 errExit 只吃单个字符串，     *
*     而原书 copy.c 写的是 errExit("opening file %s", argv[1]) —— 变参。  *
*     所以这里按原书签名实现成变参版本。别跨章复制粘贴。                  *
*                                                                         *
*  原书结构是「tlpi_hdr.h 声明 + error_functions.c / get_num.c 实现」；    *
*  为单文件可控，这里把实现做成 static inline 放在头里。语义照原书：       *
*    errExit(fmt, ...)     → 打 fmt + ": " + strerror(errno)，exit(1)     *
*    fatal(fmt, ...)       → 打 fmt，不带 errno，exit(1)                  *
*    usageErr(fmt, ...)    → 打 "Usage: " + fmt，exit(1)                  *
*    cmdLineErr(fmt, ...)  → 打 "Command-line usage error: " + fmt, 退出  *
*    getLong(arg, flags, name) → 带进制/范围校验的整数解析（get_num.c）   *
*  只实现 Ch04 用到的这几个。缺什么自己补，别以为它全。                    *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

/* 原书 error_functions.c：errExit —— 带 errno 的致命退出 */
static inline void errExit(const char *format, ...)
{
    va_list ap;

    fflush(stdout);                 /* 别让 stdout 里已缓冲的内容被 exit 丢掉 */
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* 原书 error_functions.c：fatal —— 不带 errno 的致命退出 */
static inline void fatal(const char *format, ...)
{
    va_list ap;

    fflush(stdout);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* 原书 error_functions.c：usageErr —— 打用法后退出 */
static inline void usageErr(const char *format, ...)
{
    va_list ap;

    fflush(stdout);
    fprintf(stderr, "Usage: ");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* 原书 error_functions.c：cmdLineErr —— 命令行参数非法（seek_io.c 用到） */
static inline void cmdLineErr(const char *format, ...)
{
    va_list ap;

    fflush(stdout);
    fprintf(stderr, "Command-line usage error: ");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* 原书 get_num.h + get_num.c：getLong —— 带进制/范围校验的整数解析 */
#define GN_NONNEG       01              /* 不许负数 */
#define GN_GT_0         02              /* 必须 > 0 */
#define GN_ANY_BASE     0100            /* 进制由前缀决定（0x/0） */
#define GN_BASE_8       0200            /* 强制八进制 */
#define GN_BASE_16      0400            /* 强制十六进制 */

static inline long getLong(const char *arg, int flags, const char *name)
{
    long res;
    char *endp;
    int base = (flags & GN_ANY_BASE) ? 0
             : (flags & GN_BASE_8)    ? 8
             : (flags & GN_BASE_16)   ? 16 : 10;

    errno = 0;
    res = strtol(arg, &endp, base);
    if (errno != 0 || *endp != '\0')
        fatal("Invalid number: %s", arg);   /* 原书报的是 "Could not convert ..." */
    if ((flags & GN_NONNEG) && res < 0)
        fatal("Negative value not allowed: %s", name);
    if ((flags & GN_GT_0) && res <= 0)
        fatal("Value must be > 0: %s", name);
    return res;
}

#endif /* TLPI_HDR_H */
