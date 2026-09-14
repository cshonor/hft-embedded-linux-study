/*
 * c1_4_hypothesis_div.c —— 1.4「假设 → 验证 → 收敛」的实体：一个被证伪的假设
 *
 * 起点是一句人人都信的「常识」：
 *     假设 H1：整数除零一定会发 SIGFPE 把进程干掉。
 *
 * 这份程序用三种写法各跑一次，H1 当场被证伪 —— 然后靠「看汇编」修正假设。
 * 完整推演见 1.4 笔记的「动手」一节。
 *
 * 用法：./c1_4_hypothesis_div <a> <b> <A|B|C>
 *         A：常量分子 / volatile 除零   —— 假设的漏洞在这里
 *         B：volatile 分子 / volatile 除零
 *         C：argv 传来的运行时整除
 *
 * 编译：gcc -g -O0 -Wall -Wextra -o c1_4_hypothesis_div c1_4_hypothesis_div.c
 *
 * 实测（gcc 13.3.0, -g -O0 -Wall -Wextra）：
 *   ./c1_4_hypothesis_div 1 0 A   → 打印 "A: 1 / volatile-zero = 0"，退出 0   ★ 不崩！
 *   ./c1_4_hypothesis_div 1 0 B   → 退出 136（SIGFPE）
 *   ./c1_4_hypothesis_div 1 0 C   → 退出 136（SIGFPE）
 */
#include <stdio.h>
#include <stdlib.h>

static volatile int g_zero = 0;
static volatile int g_one = 1;

int main(int argc, char **argv)
{
    int a = argc > 1 ? atoi(argv[1]) : 1;
    int b = argc > 2 ? atoi(argv[2]) : 0;

    if (argc > 3 && argv[3][0] == 'A') {          /* A: 常量分子 / volatile 除零 */
        int c = 1 / g_zero;
        printf("A: 1 / volatile-zero = %d\n", c);
    } else if (argc > 3 && argv[3][0] == 'B') {   /* B: 两个 volatile */
        int c = g_one / g_zero;
        printf("B: volatile-one / volatile-zero = %d\n", c);
    } else {                                      /* C: argv 传来的运行时值 */
        int c = a / b;
        printf("C: %d / %d = %d\n", a, b, c);
    }

    printf("没有崩，正常退出\n");
    return 0;
}
