/* c6_3_addr_space.c — 6.3 进程虚拟地址空间：各段落点 + /proc/self/maps 交叉验证
 * 编译: gcc -O2 -Wall -Wextra -o c6_3_addr_space c6_3_addr_space.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* 落在只读数据段（.rodata）的字面量 */
const char *ro_lit = "hello";

/* .data：显式初始化 */
int g_init = 7;
static int s_init = 100;

/* .bss：未初始化（或显式 0），加载时清零，磁盘上几乎不占空间 */
int g_zero;
static int s_zero;
static int s_expl_zero = 0;      /* 显式 = 0 也进 BSS */

int main(void)
{
    int stack_var = 42;                          /* 栈 */
    int *heap_var = malloc(sizeof *heap_var);    /* 堆 */
    *heap_var = 99;

    printf("=== 变量地址（低 -> 高）===\n");
    printf("  main 函数体    %p   <.text 代码段>\n", (void *)main);
    printf("  字面量 \"hello\" %p   <.rodata 只读>\n", (void *)ro_lit);
    printf("  g_init         %p   <.data 已初始化>\n", (void *)&g_init);
    printf("  s_init         %p   <.data>\n", (void *)&s_init);
    printf("  g_zero         %p   <.bss 未初始化, 值=%d>\n", (void *)&g_zero, g_zero);
    printf("  s_zero         %p   <.bss, 值=%d>\n", (void *)&s_zero, s_zero);
    printf("  s_expl_zero    %p   <.bss 显式=0 也进 BSS>\n", (void *)&s_expl_zero);
    printf("  heap(malloc)   %p   <堆, 值=%d>\n", (void *)heap_var, *heap_var);
    printf("  stack_var      %p   <栈>\n", (void *)&stack_var);

    printf("\n=== /proc/self/maps 前 8 行（内核眼中的真相）===\n");
    FILE *f = fopen("/proc/self/maps", "r");
    if (f) {
        char line[512];
        for (int i = 0; i < 8 && fgets(line, sizeof line, f); i++)
            printf("  %s", line);
        fclose(f);
    }
    free(heap_var);
    return 0;
}
