/*
 * 故意制造 use-after-free，用于 GDB 练习（Linux/WSL）
 * gdb ./demo06_dangling → run → bt → x/8xb ptr
 */
#include <stdio.h>
#include <stdlib.h>

static int *make_dangling(void)
{
    int *p = malloc(sizeof(int));
    *p = 0xDEADBEEF;
    free(p);
    return p; /* 悬垂指针：未置 NULL */
}

int main(void)
{
    int *wild = (int *)0x12345678; /* 野指针 */
    int *dangling = make_dangling();

    printf("wild=%p dangling=%p\n", (void *)wild, (void *)dangling);
    printf("解引用 wild 或 dangling 会 SIGSEGV — 取消注释一行复现:\n");
    /* printf("wild=%d\n", *wild); */
    /* printf("dangling=%d\n", *dangling); */

    puts("规范: free 后置 NULL; 入参指针先判空");
    return 0;
}
