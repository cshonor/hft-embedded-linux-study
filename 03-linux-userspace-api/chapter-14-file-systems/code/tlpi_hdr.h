/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                        *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch14 程序」里用到它的那四个     *
*  （`t_mount.c` = Listing 14-1 / `t_statfs.c` / `t_statvfs.c` /           *
*  `t_umount.c`）能零改动编译而存在。                                      *
*                                                                         *
*  原书真实文件有两层，本仓库都没有随附：                                  *
*    lib/tlpi_hdr.h         —— 声明层（**Listing 3-1**，见 Ch03 笔记）     *
*    lib/error_functions.c  —— 实现层（**Listing 3-3**）                  *
*    lib/get_num.h / .c     —— 数值解析（Listing 3-5 / 3-6）              *
*  三者都可以从 man7 直接取：                                              *
*    https://man7.org/tlpi/code/online/dist/lib/tlpi_hdr.h                *
*                                                                         *
*  Ch14 与 Ch13 替身的**差别**（别跨章复制）：                              *
*    Ch13 的四个原书程序用了 `min(m,n)` 宏与 `getLong()`，所以那一版替身     *
*    必须带上 `#include "get_num.h"` 与 min/max 宏。                       *
*    Ch14 的四个原书程序**一个都不用**（t_mount.c 的数值/短选项解析全走      *
*    getopt(3)，没有 getLong）；所以本替身**不带 get_num.h、不带 min/max**。 *
*    → 本目录下没有 `get_num.h`，这是有意的，不是漏了。                     *
*                                                                         *
*  本章的替身与真实实现的**差异**（写在这里免得读者对不上输出）：          *
*    1) errExit / fatal 的报文格式：原书打                                  *
*         ERROR [EPERM Operation not permitted] mount                      *
*       它靠 lib/ename.c.inc 的 errno 符号表把数字翻成助记名；替身没有这张 *
*       表，退化成 errno 的 strerror 文本。⚠️ **本章这一差异一定会被看到**： *
*       `t_mount.c` / `t_umount.c` 在容器里必然失败（无 CAP_SYS_ADMIN），    *
*       所以实测输出会是 `mount: Operation not permitted` 而不是           *
*       `ERROR [EPERM Operation not permitted] mount`。笔记里已标注。      *
*    2) 只提供 errExit / fatal / usageErr / cmdLineErr 四个；原书的          *
*       errMsg / err_exit / errExitEN / terminate 未提供（本章用不到）。    *
*    3) `usageErr` 的**提示文案与真实实现逐字相同**（"Usage: "），          *
*       而 `cmdLineErr` 是 "Command-line usage error: "。本章四个原书程序    *
*       在 `argc` 不对或 `--help` 时都调 `usageErr`，所以必须一致。         *
*    4) 原书 lib/tlpi_hdr.h 里的 Boolean / TRUE / FALSE / max() /           *
*       socklen_t 兜底 / O_ASYNC 兜底等一大堆可移植性补丁，本替身**不提供**  *
*       （本章四个程序都不用）。                                            *
*                                                                         *
*  ⚠️ 与 Ch04 / Ch05 / Ch10 / Ch11 / Ch12 / Ch13 的同名替身**不通用**。     *
*                                                                         *
*  原书结构是「tlpi_hdr.h 声明 + error_functions.c 实现」；为单文件可控，   *
*  这里把实现做成 static inline 放在头里。                                 *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

/* 原书 error_functions.c:101-111：errExit —— 带 errno 的致命退出
   （Ch14 四个原书程序全用它，全都是 `errExit("mount")` 这种单参数形式） */
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

/* 原书 error_functions.c:157-167：fatal —— 不带 errno 的致命退出
   （Ch14 的四个原书程序都不用，留着让替身形状与其它章一致） */
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

/* 原书 error_functions.c:171-185：usageErr —— 打用法后退出
   （t_statfs.c / t_statvfs.c / t_umount.c 都是 `argc != 2 || --help` 就调它）
   文案 "Usage: " 与原书逐字一致 */
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

/* 原书 error_functions.c:190-204：cmdLineErr —— 命令行参数错误后退出
   Ch14 四个原书程序都没用它（t_mount.c 用的是自己文件内的 static
   `usageError()`，报文格式完全不同 —— 见那个文件 :51-96）。
   留着它是为了与其它章的替身形状一致，并方便对比两种报文的差别 */
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

#endif /* TLPI_HDR_H */
