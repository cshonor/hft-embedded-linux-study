/*************************************************************************\
*  ⚠️ 这不是原书的 lib/get_num.h！                                        *
*                                                                         *
*  这是它的**最小替身**，只为本目录下「原书 Ch13 程序」里用到 `getLong()`  *
*  的那两个（`direct_read.c` / `write_bytes.c`）能零改动编译而存在。      *
*                                                                         *
*  原书真实文件是两层：                                                    *
*    lib/get_num.h  —— 声明层（**Listing 3-5**，见 Ch03 笔记）             *
*    lib/get_num.c  —— 实现层（**Listing 3-6**），含 static 的 getNum() /  *
*                      gnFail() 与 getLong() / getInt()                   *
*  两者都可以从 man7 直接取：                                              *
*    https://man7.org/tlpi/code/online/dist/lib/get_num.h                 *
*    https://man7.org/tlpi/code/online/dist/lib/get_num.c                 *
*                                                                         *
*  原书结构是「声明 + 实现」；为单文件可控，这里把实现做成 static inline   *
*  放在头里（与 Ch12 的 ugid_functions.h 同一做法）。                      *
*                                                                         *
*  与真实实现的**差异**（写在这里免得读者对不上输出）：                    *
*    1) 只提供 getLong() / getInt() 两个函数，宏定义（GN_*）**逐字相同**；  *
*    2) gnFail() 的报文格式**与原文逐字一致**（见下方实现里的注释）——     *
*       因为 Ch13 的 `direct_read.c` / `write_bytes.c` 在参数错时会真的     *
*       走到这条分支（`--help` 之外的错误参数、数值非法等），必须一致；   *
*    3) 原书 getInt() 多一道 `INT_MAX/INT_MIN` 越界检查，这里也保留了。    *
*                                                                         *
*  ⚠️ 与 Ch04 / Ch05 的同名替身**不能互换**（那两份是当年的最小集，        *
*     没有保留 gnFail 的逐字报文）。别跨章复制粘贴。                       *
\*************************************************************************/
#ifndef GET_NUM_H
#define GET_NUM_H

/* 原书把这几行放在 lib/get_num.c 里；本替身是 header-only，所以必须在头里带上
  （INT_MAX / INT_MIN 来自 <limits.h>，strtol 来自 <stdlib.h>） */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define GN_NONNEG       01      /* Value must be >= 0 */
#define GN_GT_0         02      /* Value must be > 0 */

                                /* By default, integers are decimal */
#define GN_ANY_BASE   0100      /* Can use any base - like strtol(3) */
#define GN_BASE_8     0200      /* Value is expressed in octal */
#define GN_BASE_16    0400      /* Value is expressed in hexadecimal */

/* 原书 lib/get_num.c:29-40：gnFail —— 打诊断后直接 exit(EXIT_FAILURE)。
   报文**逐字对齐**原书（注意 " error" 前面没有冒号、两行缩进是 8 个空格）：
     getLong error (in length): nonnumeric characters
             offending text: 12abc
   若 arg 为 NULL 或空串则不打第二行。 */
static inline void gnFail(const char *fname, const char *msg,
                          const char *arg, const char *name)
{
    fprintf(stderr, "%s error", fname);
    if (name != NULL)
        fprintf(stderr, " (in %s)", name);
    fprintf(stderr, ": %s\n", msg);
    if (arg != NULL && *arg != '\0')
        fprintf(stderr, "        offending text: %s\n", arg);

    exit(EXIT_FAILURE);
}

/* 原书 lib/get_num.c:46-82：getNum —— getLong() / getInt() 的公共实现。
   四道检查的**顺序**很关键（照抄原文）：
     ① NULL 或空串          → "null or empty string"
     ② strtol 后 errno != 0 → "strtol() failed"（溢出 ERANGE）
     ③ *endptr != '\0'      → "nonnumeric characters"（尾部有垃圾）
     ④ GN_NONNEG / GN_GT_0  → 范围检查
   base 的选择也是原文那串三目：
     GN_ANY_BASE → 0（由 strtol 自己按 0x/0 前缀判进制）
     GN_BASE_8   → 8 ; GN_BASE_16 → 16 ; 否则 10 */
static inline long getNum(const char *fname, const char *arg,
                          int flags, const char *name)
{
    long res;
    char *endptr;
    int base;

    if (arg == NULL || *arg == '\0')
        gnFail(fname, "null or empty string", arg, name);

    base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8) ? 8 :
                        (flags & GN_BASE_16) ? 16 : 10;

    errno = 0;
    res = strtol(arg, &endptr, base);
    if (errno != 0)
        gnFail(fname, "strtol() failed", arg, name);

    if (*endptr != '\0')
        gnFail(fname, "nonnumeric characters", arg, name);

    if ((flags & GN_NONNEG) && res < 0)
        gnFail(fname, "negative value not allowed", arg, name);

    if ((flags & GN_GT_0) && res <= 0)
        gnFail(fname, "value must be > 0", arg, name);

    return res;
}

/* 原书 lib/get_num.c:87-91 */
static inline long getLong(const char *arg, int flags, const char *name)
{
    return getNum("getLong", arg, flags, name);
}

/* 原书 lib/get_num.c:96-109 —— 注意 fname 传的是 "getInt"（不是 "getNum"），
   所以越界时报文是 "getInt error (in ...): integer out of range" */
static inline int getInt(const char *arg, int flags, const char *name)
{
    long res;

    res = getNum("getInt", arg, flags, name);

    if (res > INT_MAX || res < INT_MIN)
        gnFail("getInt", "integer out of range", arg, name);

    return res;
}

#endif /* GET_NUM_H */
