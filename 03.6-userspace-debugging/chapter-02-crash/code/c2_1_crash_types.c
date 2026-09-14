/*
 * c2_1_crash_types.c —— 「崩溃」不是一种死法：六种信号，六种退出码
 *
 * 用途：Ch2 讲的工具是 gdb / coredump，但**在拿起 gdb 之前**，
 *       第一件事是分清「它是怎么死的」。这份程序把六种最常见的崩溃
 *       各做一次，每次都给一个不同的信号 —— 于是「139 / 134 / 136」这几个
 *       神秘数字变成可复现的事实。
 *
 * 用法：./c2_1_crash_types <1..6>
 *         1 = 空指针解引用        → SIGSEGV(11)  退出码 139
 *         2 = abort()             → SIGABRT(6)   退出码 134
 *         3 = assert 失败         → SIGABRT(6)   退出码 134
 *         4 = 整数除零            → SIGFPE(8)    退出码 136
 *         5 = 栈溢出（写穿局部数组）→ SIGABRT(6)   退出码 134（栈保护金丝雀）
 *         6 = 无限递归（爆栈）     → SIGSEGV(11)  退出码 139
 *
 * 编译：
 *   gcc -g -O0 -Wall -Wextra -fstack-protector-all -o c2_1_crash_types c2_1_crash_types.c
 *
 * ⚠️ -fstack-protector-all 必须显式加：case 5 靠「栈保护金丝雀」才变成
 *    SIGABRT + 一条 *** stack smashing detected *** 的消息，不加则可能
 *    静默写坏栈、或者过一会儿才在别处崩（那才是最难查的形态）。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile int g_sink;

/* ---- case 1：空指针解引用 ---- */
static int crash_null(void)
{
    struct order { long id; long qty; };
    struct order *o = NULL;

    printf("case 1: 准备解引用 NULL\n");
    fflush(stdout);
    o->qty = 100;                       /* ← SIGSEGV */
    return 0;
}

/* ---- case 2：abort() ---- */
static int crash_abort(void)
{
    printf("case 2: 主动 abort()（断言不了的严重错误）\n");
    fflush(stdout);
    abort();                            /* ← SIGABRT */
    return 0;
}

/* ---- case 3：assert 失败 ---- */
static int crash_assert(void)
{
    int qty = -5;

    printf("case 3: assert(qty > 0) 而 qty = %d\n", qty);
    fflush(stdout);
    assert(qty > 0);                    /* ← SIGABRT + 打印文件行号 */
    return 0;
}

/* ---- case 4：整数除零 ---- */
static int crash_div(void)
{
    volatile long volume = 0;
    volatile long turnover = 10700;

    printf("case 4: 算均价而成交量为 0\n");
    fflush(stdout);
    g_sink = (int)(turnover / volume);  /* ← SIGFPE */
    return 0;
}

/* ---- case 5：写穿局部数组（触发栈保护金丝雀） ---- */
static int crash_stack(void)
{
    char sym[8];

    printf("case 5: 往 char sym[8] 里拷一个 24 字节的代码\n");
    fflush(stdout);
    strcpy(sym, "AAPL240621C00150000");  /* 19 字节 + '\0' = 20 > 8，写穿金丝雀 */
    g_sink = sym[0];
    return 0;
}

/* ---- case 6：无限递归（把栈耗光） ---- */
static long recurse(long depth)
{
    volatile char pad[512];             /* 每层吃掉 512 字节栈 */
    pad[0] = (char)depth;
    g_sink = pad[0];
    return recurse(depth + 1) + pad[0]; /* ← noinline 保证不变成尾调用 */
}

static int crash_recursion(void)
{
    printf("case 6: 无限递归，准备把 8MB 栈耗光\n");
    fflush(stdout);
    return (int)recurse(0);
}

int main(int argc, char **argv)
{
    int which = (argc > 1) ? atoi(argv[1]) : 0;

    switch (which) {
    case 1: return crash_null();
    case 2: return crash_abort();
    case 3: return crash_assert();
    case 4: return crash_div();
    case 5: return crash_stack();
    case 6: return crash_recursion();
    default:
        fprintf(stderr, "用法: %s <1..6>\n", argv[0]);
        return 2;
    }
}
