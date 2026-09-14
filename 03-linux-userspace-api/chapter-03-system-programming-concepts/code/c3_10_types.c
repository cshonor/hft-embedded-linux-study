/* TLPI 第 03 章 §3.6.2 —— 标准系统数据类型：别猜宽度，用 sizeof 问出来
 *
 * TLPI 专门强调：`int` / `long` 的宽度随平台变，但**系统调用的接口类型**
 * （pid_t、uid_t、off_t、size_t、ssize_t…）是由实现定义好的类型。
 * 想写出可移植的 printf，就必须用 <inttypes.h> 的 PRI* 宏。
 *
 * 本 demo 用两种 OFF_T 设定各编一次，看 off_t 是否变化：
 *   gcc -O0 -Wall -Wextra -o c3_10 c3_10_types.c
 *   gcc -O0 -Wall -Wextra -D_FILE_OFFSET_BITS=64 -o c3_10 c3_10_types.c
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

/* 用泛型「打印类型宽度」的宏，避免一行行重复 */
#define W(t) printf("  %-22s sizeof = %2zu 字节   %s\n", #t, sizeof(t), kind(sizeof(t)))

static const char *kind(size_t n)
{
    switch (n) {
    case 1: return "1B";
    case 2: return "2B";
    case 4: return "4B";
    case 8: return "8B";
    case 16: return "16B";
    default: return "?";
    }
}

int main(void)
{
    printf("=== ① 基础 C 类型：这就是「数据模型」===\n");
    W(char); W(short); W(int); W(long); W(long long);
    W(float); W(double); W(long double);
    W(void *); W(size_t); W(ptrdiff_t);
    printf("  → 指针 %zu 字节 + long %zu 字节 ⇒ 数据模型 %s\n",
           sizeof(void *), sizeof(long),
           (sizeof(void *) == 8 && sizeof(int) == 4) ? "LP64（x86-64 / aarch64 用户态）" : "其它");

    printf("\n=== ② 系统调用接口类型（POSIX 只规定「够用」，不规定具体宽度）===\n");
    W(pid_t); W(uid_t); W(gid_t); W(mode_t); W(dev_t);
    W(off_t); W(ssize_t); W(time_t);
#ifdef __USE_MISC
    W(socklen_t); W(useconds_t);
#endif

    printf("\n=== ③ _FILE_OFFSET_BITS=64 的作用 ===\n");
#ifdef _FILE_OFFSET_BITS
    printf("  _FILE_OFFSET_BITS = %d\n", _FILE_OFFSET_BITS);
#else
    printf("  _FILE_OFFSET_BITS 未定义\n");
#endif
    printf("  sizeof(off_t) = %zu\n", sizeof(off_t));
    printf("  在 x86-64/aarch64 上 off_t 本来就是 64 位，这个宏**没有影响**；\n");
    printf("  它真正的用武之地是 32 位平台：把 off_t 从 32 位换成 64 位，从而能操作 >2GB 文件。\n");
    printf("  代价是所有相关函数的 ABI 都换一套（glibc 里叫 open64/lseek64）。\n");

    printf("\n=== ④ 为什么必须有 PRI* 宏：同一份代码在不同平台的可移植打印 ===\n");
    int32_t  i32 = -123456;
    uint32_t u32 = 4000000000u;
    int64_t  i64 = -1234567890123456789LL;
    uint64_t u64 = 18446744073709551615ULL;
    size_t   sz  = sizeof(int64_t);
    ssize_t  ssz = -1;
    pid_t    pid = getpid();
    off_t    off = 1 << 20;

    printf("  int32_t  : %" PRId32 "\n", i32);
    printf("  uint32_t : %" PRIu32 "\n", u32);
    printf("  int64_t  : %" PRId64 "\n", i64);
    printf("  uint64_t : %" PRIu64 "\n", u64);
    printf("  size_t   : %zu\n", sz);            /* 用 %zu 而不是 %lu */
    printf("  ssize_t  : %zd\n", ssz);           /* 用 %zd 而不是 %ld */
    printf("  pid_t    : %d（按 %zu 字节的类型打印，编译期由 glibc 保证匹配）\n",
           (int) pid, sizeof(pid_t));
    printf("  off_t    : %jd（转型到 intmax_t 最保险）\n", (intmax_t) off);

    printf("\n=== ⑤ 踩坑清单 ===\n");
    printf("  ✗ printf(\"%%d\", sizeof(x))        → size_t 是 64 位，%%d 只读 32 位，栈错位\n");
    printf("  ✗ printf(\"%%ld\", ssize_t 值)      → 在 Windows/32 位下不对，用 %%zd\n");
    printf("  ✗ 把 pid_t 当 int 传给变参函数     → 用 (int) 显式转，或 %%jd\n");
    printf("  ✗ 假设 long 是 32 位               → LP64 下是 64 位，ILP32 下才是 32 位\n");
    printf("  ✓ 需要「恰好 N 位」时用 <stdint.h> 的 intN_t/uintN_t，别用 int/long\n");
    return 0;
}
