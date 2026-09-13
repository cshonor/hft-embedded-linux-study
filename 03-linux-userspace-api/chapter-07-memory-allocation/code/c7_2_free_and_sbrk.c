/* Ch7 §7.1 — free() 不一定下调 program break（TLPI Listing 7-1 精神）
 * 编译: gcc -O2 -Wall -Wextra -o c7_2_free_and_sbrk c7_2_free_and_sbrk.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 1000

static char *g_base;

static void show(const char *tag)
{
    char *now = (char *) sbrk(0);
    printf("%-30s break = %p   (较起点 %+8ld B)\n",
           tag, (void *) now, (long) (now - g_base));
}

int main(void)
{
    static char *ptr[N];

    g_base = (char *) sbrk(0);
    show("启动");

    /* 阶段 1：大量小块 —— 走主堆 */
    for (int i = 0; i < N; i++) {
        ptr[i] = malloc(1024);
        if (ptr[i] == NULL) { perror("malloc"); return 1; }
        ptr[i][0] = 'x';            /* 触碰一下，确保真的占了页 */
    }
    show("malloc(1000 x 1KB) 之后");

    for (int i = 0; i < N; i++)
        free(ptr[i]);
    show("全部 free 之后");

    /* 阶段 2：一个大块 —— 走 mmap，与 program break 无关 */
    char *big = malloc(256 * 1024);
    if (big == NULL) { perror("malloc"); return 1; }
    big[0] = 'y';
    show("malloc(256KB) 之后");

    free(big);
    show("free(256KB) 之后");
    return 0;
}
