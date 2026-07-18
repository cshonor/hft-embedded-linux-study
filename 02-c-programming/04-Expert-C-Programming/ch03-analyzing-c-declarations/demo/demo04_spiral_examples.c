/*
 * ch03 螺旋规则速查（注释）
 *
 * char *str[10];           str: 10 元素数组，元素为 char*
 * char (*p)[10];          p: 指向「含 10 个 char 的数组」的指针
 * int (*f)(int, char);    f: 指向函数( int, char ) -> int 的指针
 *
 * char *(*(*x())[])();
 *   x: 无参函数 -> 指针 -> 数组 -> 指针 -> 无参函数 -> char*
 *
 * void (*signal(int sig, void (*handler)(int)))(int);
 *   signal: (int, void(*)(int)) -> 指针 -> 函数(int) -> void
 */
#include <stdio.h>

int main(void)
{
    puts("spiral rule cheat sheet — see comments and 3.8 notes");
    return 0;
}
