/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下 5 个「原书 Ch10 程序」能在本仓库里  *
*  零改动单独编译而存在：那几个文件都写着 #include "tlpi_hdr.h"，原书的   *
*  头文件（Listing 3-1 + 3-2 + 3-5）本仓库没有随附，见 Ch03 笔记。        *
*                                                                         *
*  原书的结构是「tlpi_hdr.h 声明 + error_functions.c / get_num.c 实现」；*
*  这里为了单文件可控，把 Ch10 真正用到的那几个符号直接做成 static inline *
*  放在头里。语义照原书：errExit/fatal/usageErr 打印到 stderr 后          *
*  exit(EXIT_FAILURE)；getInt 用 strtol + errno + endptr 做三态判定。      *
*                                                                         *
*  只实现了 Ch10 用到的：errExit / fatal / usageErr / getInt / GN_GT_0。   *
*  不要拿它去编别的章的原书程序 —— 缺什么自己补，别以为它全。             *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GN_GT_0 1

/* 原书 error_functions.c 里的 errExit(msg)：带 errno 的致命退出 */
static inline void errExit(const char *msg)
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

/* 原书 fatal(msg)：不带 errno 的致命退出 */
static inline void fatal(const char *msg)
{
    fprintf(stderr, "FATAL: %s\n", msg);
    exit(EXIT_FAILURE);
}

/* 原书 usageErr(fmt, ...)：打用法后退出 */
static inline void usageErr(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);                     /* 别让 stdout 的缓冲被 exit 丢掉 */
    fprintf(stderr, "Usage: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

/* 原书 get_num.c 里的 getInt()：strtol + errno + endptr 三态判定 */
static inline int getInt(const char *arg, int flags, const char *name)
{
    char *endptr;
    long n;

    if (arg == NULL || *arg == '\0') {
        fprintf(stderr, "ERROR: getInt(): empty string (%s)\n", name);
        exit(EXIT_FAILURE);
    }

    errno = 0;                          /* strtol 不会自己清 errno */
    n = strtol(arg, &endptr, 0);
    if (errno != 0)
        errExit("strtol");
    if (endptr == arg || *endptr != '\0') {
        fprintf(stderr, "ERROR: getInt(): not a number (%s)\n", name);
        exit(EXIT_FAILURE);
    }
    if (n > INT_MAX) {
        fprintf(stderr, "ERROR: getInt(): value too large (%s)\n", name);
        exit(EXIT_FAILURE);
    }
    if ((flags & GN_GT_0) && n <= 0) {
        fprintf(stderr, "ERROR: getInt(): value must be > 0 (%s)\n", name);
        exit(EXIT_FAILURE);
    }
    return (int) n;
}

#endif /* TLPI_HDR_H */
