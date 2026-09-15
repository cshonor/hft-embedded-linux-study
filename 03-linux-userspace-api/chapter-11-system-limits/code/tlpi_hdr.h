/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch11 程序」（t_sysconf.c /    *
*  t_fpathconf.c）能零改动编译而存在。原书的头文件散在 Listing 3-1 + 3-2  *
*  + 3-5，本仓库没有随附，见 Ch03 笔记。                                  *
*                                                                         *
*  ⚠️ 与 Ch04 / Ch05 / Ch10 的同名替身**不通用**：                        *
*     - Ch04/Ch05 那份多带 getLong/getInt（copy.c / atomic_append.c 用），*
*     - 这里 Ch11 的两个原书程序只用到 errExit()，所以只保留           *
*       errExit / fatal / usageErr 三个。                                 *
*     别跨章复制粘贴。                                                     *
*                                                                         *
*  原书结构是「tlpi_hdr.h 声明 + error_functions.c 实现」；为单文件可控，  *
*  这里把实现做成 static inline 放在头里。语义照原书：                     *
*    errExit(fmt, ...)     → 打 fmt + ": " + strerror(errno)，exit(1)     *
*    fatal(fmt, ...)       → 打 fmt，不带 errno，exit(1)                  *
*    usageErr(fmt, ...)    → 打 "Usage: " + fmt，exit(1)                  *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

/* 原书 error_functions.c：errExit —— 带 errno 的致命退出
   （t_sysconf.c / t_fpathconf.c 都写的是 errExit("sysconf %s", msg)） */
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

#endif /* TLPI_HDR_H */
