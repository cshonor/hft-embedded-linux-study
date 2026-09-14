/* TLPI 第 10 章 §10.2 Time-Conversion Functions
 * 调用原书 Listing 10-2（curr_time.c）的 currTime() 助手函数。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_2_curr_time_demo \
 *           c10_2_curr_time_demo.c curr_time.c
 * 运行: ./c10_2_curr_time_demo               (无需参数、无需权限)
 *
 * 为什么要单独一个 driver：curr_time.c 只提供函数、没有 main，是给后面章节
 * （Ch23 的定时器例子要打时间戳）复用的**库文件**。CE 只吃单文件，所以验证时
 * 用 gen_ce_single.py 把 curr_time.h + curr_time.c + 本文件拼成一个 .c 编译。
 *
 * 要看的点：
 *   * currTime(NULL) 走的是 "%c"，也就是 ctime(3) 的格式**去掉换行**；
 *   * 返回的指针指向函数内部的 static 缓冲区 —— 原书注释里的
 *     "Nonreentrant" 就是这个意思，两次调用会互相覆盖。
 */
#include <stdio.h>
#include <time.h>

#include "curr_time.h"

int main(void)
{
    char *p1, *p2;

    printf("currTime(NULL)                 -> %s\n", currTime(NULL));
    printf("currTime(\"%%F %%T\")              -> %s\n", currTime("%F %T"));
    printf("currTime(\"%%T\")                 -> %s\n", currTime("%T"));
    printf("currTime(\"%%Y%%m%%d-%%H%%M%%S\")      -> %s\n", currTime("%Y%m%d-%H%M%S"));
    printf("currTime(\"%%A %%d %%B %%Y, %%H:%%M:%%S %%Z\") -> %s\n",
           currTime("%A %d %B %Y, %H:%M:%S %Z"));
    printf("\n");

    /* 非可重入：两次调用返回同一个地址 */
    p1 = currTime("%T");
    printf("第一次 currTime(\"%%T\") 返回 %p  内容: %s\n", (void *) p1, p1);
    p2 = currTime("%F");
    printf("第二次 currTime(\"%%F\") 返回 %p  内容: %s\n", (void *) p2, p2);
    printf("两个指针相同? %s\n", (p1 == p2) ? "是 —— 同一个 static 缓冲区" : "否");
    printf("此时用第一个指针 p1 读出来是: %s   <- 已被第二次调用覆盖\n", p1);

    return 0;
}
