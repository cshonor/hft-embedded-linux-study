/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch23 程序」能零改动编译而存在： *
*    real_timer.c / timed_read.c / t_nanosleep.c / t_clock_nanosleep.c /   *
*    ptmr_sigev_signal.c / ptmr_sigev_thread.c / ptmr_null_evp.c /         *
*    demo_timerfd.c / cpu_burner.c / cpu_multi_burner.c /                  *
*    cpu_multithread_burner.c                                              *
*                                                                         *
*  原书真实文件有两层，本仓库都没有随附：                                  *
*    lib/tlpi_hdr.h         —— 声明层（Listing 3-1/3-2/3-5，见 Ch03 笔记） *
*    lib/error_functions.c  —— 实现层（errMsg / errExit / errExitEN /     *
*                              fatal / usageErr / cmdLineErr / terminate）*
*  两者都可以从 man7 直接取：                                              *
*    https://man7.org/tlpi/code/online/dist/lib/tlpi_hdr.h                *
*    https://man7.org/tlpi/code/online/dist/lib/error_functions.c         *
*  原书结构是「tlpi_hdr.h 声明 + error_functions.c 实现」；为单文件可控，  *
*  这里把实现做成 static inline 放在头里，**报文格式逐字对齐官方**。       *
*                                                                         *
*  ✅ 与官方逐字一致、且本章真能跑到的三处：                                *
*     1. usageErr 的 "Usage: "、cmdLineErr 的 "Command-line usage error: " *
*        —— 纯字符串、不依赖 ename 表，且 real_timer / t_nanosleep /       *
*        t_clock_nanosleep / ptmr_* / demo_timerfd 参数不对时会真的触发。  *
*     2. Boolean 的类型定义：官方是 `typedef enum { FALSE, TRUE } Boolean;` *
*        且**先 #undef 掉**某些实现已定义的 TRUE/FALSE（见官方 tlpi_hdr.h）。*
*        这里照抄。                                                        *
*     3. outputError() 里那条会被 gcc 报 -Wformat-truncation 的 snprintf —— *
*        官方用 `#pragma GCC diagnostic push/ignored/pop` 把它压掉，这里    *
*        **同样压掉**（否则会凭空多出一条官方没有的警告）。                *
*                                                                         *
*  ✅ 与官方唯一的偏差（就是下面那张 ename 表）：                           *
*     outputError() 里 `[%s %s]` 的第一个 %s 在原书取自 `lib/ename.c.inc`  *
*     —— 那是一张由 `lib/build_ename.sh` 从 errno.h **生成**的 errno 助记名 *
*     表（`ENOENT` / `EINVAL` …）。本替身没有这张表，退化成 `?UNKNOWN?`：  *
*                                                                         *
*       原书：ERROR [ENOENT No such file or directory] clock_gettime       *
*       替身：ERROR [?UNKNOWN? No such file or directory] clock_gettime    *
*                                                                         *
*     本章各程序里 errExit/errExitEN 只在「clock_gettime / timer_* /       *
*     sigaction 失败」这类不该发生的分支上才走 ⇒ **正常运行路径看不到差异**。*
*     顺带说明：这也是为什么书上的报错文本能显示 `ENOENT` 这类名字——它不是 *
*     手写的，是 build_ename.sh 生成的。                                    *
*                                                                         *
*  ⚠️ 官方 tlpi_hdr.h 里另有几段**平台兼容块**，本替身按需省略：            *
*     `socklen_t`（__sgi）/ `FASYNC→O_ASYNC` / `MAP_ANON→MAP_ANONYMOUS` /   *
*     `O_FSYNC→O_SYNC` / `__FreeBSD__` 的 sigval 字段别名 ——               *
*     这些在 Linux/glibc 上全是空操作（已逐条比对官方原件），本章无程序用到。*
*                                                                         *
*  ⚠️ 与本仓库**其它章的同名替身不通用，且报文格式不同**：                  *
*     Ch10–Ch22 那份把 errExit 写成 `msg: strerror` 直出（没有 `ERROR`     *
*     前缀、没有 `[...]` 段）；本 Ch23 份**按官方格式**输出 `ERROR [...]`。*
*     各章 notebooks 引用的报错文本因此不同，别跨章对照。                  *
*                                                                         *
*  ⚠️ 依赖边界（本章最小化，已逐个 grep 确认）：                            *
*     - get_num.h  ：real_timer / t_nanosleep / t_clock_nanosleep 用        *
*                    getLong；timed_read / demo_timerfd 用 getInt。        *
*     - stdbool.h  ：cpu_burner / cpu_multi_burner /                         *
*                    cpu_multithread_burner 用 `bool`（**官方 tlpi_hdr.h    *
*                    也含它**，已核对原件）。                                *
*     - max()      ：官方 tlpi_hdr.h 里有 min/max，但**本章没有程序调用**   *
*                    （demo_timerfd 用的是变量名 maxExp，不是宏）。这里仍旧  *
*                    保留，以便与官方头对齐。                                *
*     - curr_time.h：不在本头里，由 ptmr_* 三个程序自己 #include。          *
*     - **不含 signal_functions.h** —— 本章没有程序用 printSigMask。        *
*     - err_exit / 不带 errno 的 terminate：本章无程序用，不提供。          *
\*************************************************************************/
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* EXIT_SUCCESS / EXIT_FAILURE, malloc, atoi... */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */
#include <stdbool.h>    /* 'bool' type plus 'true' and 'false' constants */
#include <stdarg.h>

#include "get_num.h"    /* Declares getInt() / getLong() */

/* 原书：先把某些实现已定义的 TRUE/FALSE 撤掉，再用枚举定义自己的 Boolean */
#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

typedef enum { FALSE, TRUE } Boolean;

/* 原书 tlpi_hdr.h 的 min/max（本章实际无人调用，保留以对齐官方头） */
#define min(m,n) ((m) < (n) ? (m) : (n))
#define max(m,n) ((m) > (n) ? (m) : (n))

/* 原书 error_functions.c:26-38：terminate —— EF_DUMPCORE 环境变量非空则
   abort() 产生 core，否则按 useExit3 选 exit()（会刷 stdio）还是 _exit() */
static inline void terminate(Boolean useExit3)
{
    char *s;

    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

/* 原书 error_functions.c:49-78：outputError —— 唯一偏差是 ename[err] 退化成
   "?UNKNOWN?"（见文件头说明） */
static inline void outputError(Boolean useErr, int err, Boolean flushStdout,
                               const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]", "?UNKNOWN?", strerror(err));
    else
        snprintf(errText, BUF_SIZE, ":");

    /* 官方 error_functions.c 就在这里 push/ignored/pop 掉 -Wformat-truncation：
       errText 与 userMsg 各自都是 BUF_SIZE 上限，拼起来必然「可能被截断」，
       作者认为这是可接受的设计，故显式静音。本替身照抄，否则会多出一条
       **官方代码里没有的**警告。 */
#if __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);
#if __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif

    if (flushStdout)
        fflush(stdout);       /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr);           /* In case stderr is not line-buffered */
#undef BUF_SIZE
}

/* 原书 error_functions.c:83-96：errMsg —— 报错但不退出（timed_read.c 用它
   处理「read 既非 EINTR 也非正常」的非致命错），返回前**恢复 errno** */
static inline void errMsg(const char *format, ...)
{
    va_list argList;
    int savedErrno;

    savedErrno = errno;       /* In case we change it here */

    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);

    errno = savedErrno;
}

/* 原书 error_functions.c:101-111：errExit —— 带 errno 的致命退出（本章 14 个
   程序里 11 个都用它） */
static inline void errExit(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);

    terminate(TRUE);
}

/* 原书 error_functions.c:144-153：errExitEN —— 用**指定的** errnum 报错后退出
   （cpu_multithread_burner / ptmr_sigev_thread / t_clock_nanosleep 用） */
static inline void errExitEN(int errnum, const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    outputError(TRUE, errnum, TRUE, format, argList);
    va_end(argList);

    terminate(TRUE);
}

/* 原书 error_functions.c:157-167：fatal —— 不带 errno 的致命退出
   （cpu_burner.c 在 idle-percent 越界等分支上用它） */
static inline void fatal(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    outputError(FALSE, 0, TRUE, format, argList);
    va_end(argList);

    terminate(TRUE);
}

/* 原书 error_functions.c:171-185：usageErr —— 打用法后退出（与官方逐字一致） */
static inline void usageErr(const char *format, ...)
{
    va_list argList;

    fflush(stdout);           /* Flush any pending stdout */

    fprintf(stderr, "Usage: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr);           /* In case stderr is not line-buffered */
    exit(EXIT_FAILURE);
}

/* 原书 error_functions.c:190-204：cmdLineErr —— 命令行参数错误后退出
   （get_num.c 的 getInt/getLong 解析失败时调它；与官方逐字一致） */
static inline void cmdLineErr(const char *format, ...)
{
    va_list argList;

    fflush(stdout);           /* Flush any pending stdout */

    fprintf(stderr, "Command-line usage error: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr);           /* In case stderr is not line-buffered */
    exit(EXIT_FAILURE);
}

#endif /* TLPI_HDR_H */
