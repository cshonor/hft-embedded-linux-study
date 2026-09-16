/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch20 程序」（ouch.c /        *
*  intquit.c / t_kill.c / sig_sender.c / sig_receiver.c /                 *
*  signal_functions.c 以及 14 个补充程序）能零改动编译而存在。             *
*                                                                         *
*  原书真实文件有两层，本仓库都没有随附：                                  *
*    lib/tlpi_hdr.h         —— 声明层（Listing 3-1/3-2/3-5，见 Ch03 笔记） *
*    lib/error_functions.c  —— 实现层（errExit / fatal / usageErr /       *
*                              cmdLineErr / errMsg / err_exit / errExitEN）*
*  两者都可以从 man7 直接取：                                              *
*    https://man7.org/tlpi/code/online/dist/lib/tlpi_hdr.h                *
*  本章的替身与真实实现的**差异**（写在这里免得读者对不上输出）：          *
*    1) errExit/fatal 的报文格式：原书打                                    *
*         ERROR [ENOENT No such file or directory] open                    *
*       它靠 lib/ename.c.inc 的 errno 符号表把数字翻成助记名；替身没有这张 *
*       表，退化成 errno 的 strerror 文本。本章各程序里 errExit 只在       *
*       「注册 handler / sigprocmask 失败」这类不该发生的分支上才走，       *
*       正常运行路径看不到差异。                                           *
*    2) 只提供 errExit / fatal / usageErr / cmdLineErr / errMsg；原书的     *
*       err_exit / errExitEN / terminate 未提供（本章用不到）。             *
*    3) usageErr 的**提示文案与真实实现逐字相同**（"Usage: "），因为它    *
*       是纯字符串，不依赖 ename表 —— t_kill.c / sig_sender.c 等都会触发。  *
*                                                                         *
*  ⚠️ 与 Ch04 / Ch05 / Ch10 / Ch11 等章的同名替身**不通用**：              *
*     - 本 Ch20 那份含 get_num.h（t_kill/sig_sender/sig_receiver 用        *
*       getInt/getLong/getNum 解析命令行），与 Ch18/Ch19 那份同构。        *
*     别跨章复制粘贴。                                                     *
*                                                                         *
*  原书结构是「tlpi_hdr.h 声明 + error_functions.c 实现」；为单文件可控，  *
*  这里把实现做成 static inline 放在头里。                                 *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include "get_num.h"   /* 原书 tlpi_hdr.h 也含它；t_kill/sig_sender/sig_receiver 用 */

/* 原书头里的 Boolean（t_stat.c 用到）； mac 上 enum 常量冲突，用 int+宏 */
#define TRUE 1
#define FALSE 0
typedef int Boolean;

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

/* 原书 error_functions.c:101-111：errExit —— 带 errno 的致命退出
   （本章各程序注册 handler / 改掩码失败时调它） */
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
   （procfs_pidmax.c 在 write 短写时调它） */
/* errMsg —— 记录错误但不退出（t_umask.c 用它处理「清理失败」非致命错） */
static inline void errMsg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
}

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
   （t_kill.c / sig_sender.c / sig_receiver.c / t_sigqueue.c 等在 argc 不对时调它）
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
   （get_num.c 的 getNum/getInt/getLong 在解析失败时调它）
   文案 "Command-line usage error: " 与原书逐字一致 */
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
