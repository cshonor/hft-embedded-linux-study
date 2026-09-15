/*************************************************************************\
*  ⚠️ 这不是原书的 lib/tlpi_hdr.h！                                      *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch13 程序」里用到它的那两个    *
*  （`direct_read.c` = Listing 13-1 / `write_bytes.c`）能零改动编译而存在。*
*                                                                         *
*  原书真实文件有两层，本仓库都没有随附：                                  *
*    lib/tlpi_hdr.h         —— 声明层（**Listing 3-1**，见 Ch03 笔记）     *
*    lib/error_functions.c  —— 实现层（**Listing 3-3**）                  *
*    lib/get_num.h / .c     —— 数值解析（Listing 3-5 / 3-6）              *
*  三者都可以从 man7 直接取：                                              *
*    https://man7.org/tlpi/code/online/dist/lib/tlpi_hdr.h                *
*                                                                         *
*  Ch13 的**特别之处**（与 Ch04~Ch12 各章的替身都不同）：                  *
*    原书 `write_bytes.c` 用了两个东西：                                    *
*      ① `min(m, n)` 宏 —— 它定义在 lib/tlpi_hdr.h:45（本替身逐字照抄）；    *
*      ② `getLong(arg, GN_GT_0, "num-bytes")` —— 它来自 lib/get_num.h，    *
*         而 lib/tlpi_hdr.h:27 就是靠 `#include "get_num.h"` 把它带进来的。 *
*    所以本替身**必须**同时提供 min 与 get_num.h，否则 write_bytes.c 无法   *
*    零改动编译。Ch04 / Ch05 的替身有 getLong 但**没有 min**，不能互换。    *
*                                                                         *
*  本章的替身与真实实现的**差异**（写在这里免得读者对不上输出）：          *
*    1) errExit / fatal 的报文格式：原书打                                  *
*         ERROR [ENOENT No such file or directory] open                    *
*       它靠 lib/ename.c.inc 的 errno 符号表把数字翻成助记名；替身没有这张 *
*       表，退化成 errno 的 strerror 文本。Ch13 的 `--help` / 参数错都走     *
*       usageErr 而不是 errExit，所以这个差异在本章实测里看不到。           *
*    2) 只提供 errExit / fatal / usageErr / cmdLineErr 四个；原书的          *
*       errMsg / err_exit / errExitEN / terminate 未提供（本章用不到）。     *
*    3) usageErr / cmdLineErr 的**提示文案与真实实现逐字相同**              *
*       （"Usage: " / "Command-line usage error: "），本章会真的触发 usageErr *
*       （两个原书程序都是 `argc < 3 || --help` 就调它），所以必须一致。     *
*    4) 原书 lib/tlpi_hdr.h 里的 Boolean / TRUE / FALSE / max() /           *
*       socklen_t 兜底 / O_ASYNC 兜底等一大堆可移植性补丁，本替身**不提供**  *
*       （Ch13 两个程序都不用）。                                          *
*                                                                         *
*  ⚠️ 与 Ch04 / Ch05 / Ch10 / Ch11 / Ch12 的同名替身**不通用**，别跨章复制。 *
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

/* 原书 lib/tlpi_hdr.h:27 就是这一行把 getLong() / GN_* 带进来的 */
#include "get_num.h"

/* 原书 lib/tlpi_hdr.h:45-46（逐字照抄）—— write_bytes.c 的
   `thisWrite = min(bufSize, numBytes - totWritten);` 就靠它 */
#define min(m,n) ((m) < (n) ? (m) : (n))
#define max(m,n) ((m) > (n) ? (m) : (n))

/* 原书 error_functions.c:101-111：errExit —— 带 errno 的致命退出
   （direct_read.c / write_bytes.c 都写的是 errExit("...")） */
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
   （write_bytes.c 在 write 短写时调它，报文 "partial/failed write"） */
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
   （direct_read.c / write_bytes.c 都在 argc 不对或 --help 时调它）
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
   本章两个原书程序都没用它，但留着它可以让「替身与 Ch12 那份形状一致」，
   也方便读者对比它与 usageErr 的差别（只差前缀，**没有**再打一遍 "Usage:"） */
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
