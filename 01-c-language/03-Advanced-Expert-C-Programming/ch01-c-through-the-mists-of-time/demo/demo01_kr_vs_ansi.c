/* demo01: K&R 旧式声明 vs ANSI 原型 — 编译对比 */
#include <stdio.h>

/* ANSI 原型：编译器可校验实参 */
static int add_ansi(int a, double b)
{
    return (int)(a + b);
}

/*
 * K&R 旧式（仅作历史演示，勿在新代码中使用）：
 * int add_kr(a, b) int a; double b; { return a + b; }
 */

int main(void)
{
    printf("add_ansi(1, 2.5) = %d\n", add_ansi(1, 2.5));
    /* add_ansi("x", 1);  // 取消注释：ANSI 下编译失败 */
    return 0;
}
