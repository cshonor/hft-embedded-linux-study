/*
 * c3_3_ubsan_ops.c —— UBSan 能抓到的「看不见的错」：算错但程序不崩
 *
 * 用途：3.3 的核心论点 —— 未定义行为（UB）不是「跑起来崩」，
 *       而是「跑起来一切正常，只是结果是错的」。这份程序把所有输入
 *       都放进 volatile 全局，逼编译器**真去执行**这些运算，
 *       于是 UBSan 能在运行时把每一处 UB 逐条点名。
 *
 * 编译（UBSan，默认 recover 模式：报错后继续跑，把错列全）：
 *   gcc -g -O1 -fsanitize=undefined -o c3_3_ubsan c3_3_ubsan_ops.c
 * 编译（报错即停，便于定位第一个雷）：
 *   gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=all -o c3_3_ubsan_stop c3_3_ubsan_ops.c
 * 不加 sanitizer 的对照：
 *   gcc -g -O0 -Wall -Wextra -o c3_3_plain c3_3_ubsan_ops.c
 */
#include <stdio.h>
#include <stdlib.h>

static volatile int v_int_max = 2147483647;      /* INT_MAX */
static volatile int v_one = 1;
static volatile int v_shift = 40;
static volatile int v_zero = 0;
static volatile long v_min = -9223372036854775807L - 1;   /* LONG_MIN */

int main(void)
{
    /* 每行 printf 都跟一个 fflush：④ 除零会发 SIGFPE 直接杀进程，
     * 而 stdout 接管道时是全缓冲的 —— 不刷的话前面四条「看似合理的错值」
     * 全留在缓冲区里，随进程一起消失，你什么都看不到。见 3.3 笔记的实测。 */
    int a = v_int_max;
    int overflow = a + v_one;                     /* ① 有符号溢出：UB */
    printf("① INT_MAX + 1 = %d   （数学上是 2147483648，int 装不下）\n", overflow);
    fflush(stdout);

    int b = v_one << v_shift;                     /* ② 移位量 ≥ 位宽：UB */
    printf("② 1 << 40 = %d          （int 只有 32 位）\n", b);
    fflush(stdout);

    long d = v_min;
    long negate = -d;                             /* ③ 对 LONG_MIN 取负：UB */
    printf("③ -LONG_MIN = %ld\n", negate);
    fflush(stdout);

    long shift_long = (long)v_one << v_shift;     /* 对照：long 装得下，合法 */
    printf("对照：1L << 40 = %ld（合法，没有 UB）\n", shift_long);
    fflush(stdout);

    /* ④ 除零：唯一会**立刻发 SIGFPE 终止进程**的 UB，所以放在最后。
     *    两个操作数都必须是运行时值 —— 写成 `1 / v_zero`（常量分子）时
     *    gcc 13.3 -O0 会按「除数是 0 属于 UB，可以假定非零」把它折成不含除法的
     *    比较序列，结果是「不崩、直接打出 0」。详见 3.3 笔记的实测对照。 */
    volatile int one = v_one;
    volatile int zero = v_zero;
    int c = one / zero;
    printf("④ 1 / 0 = %d           （能打出来才怪）\n", c);
    fflush(stdout);

    return 0;
}
