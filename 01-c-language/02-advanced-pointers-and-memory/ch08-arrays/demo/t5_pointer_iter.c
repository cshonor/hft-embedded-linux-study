/* t5_pointer_iter.c — 指针遍历数组：核心等价 / 尾后指针 / 关系比较
 *
 * 对应笔记 8.1.3 指针与下标
 * 环境：WSL Ubuntu，gcc 13.3 / clang 18.1.3
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stddef.h>

static int g[5] = {10, 20, 30, 40, 50};

/* noinline：阻断编译器把两个无关局部数组的关系比较折叠成常量 */
__attribute__((noinline))
static int raw_lt(const int *a, const int *b) { return a < b; }

static void sec(const char *t) { printf("\n--- %s ---\n", t); }

int main(void)
{
    const int n = 5;

    /* T5.1 两种遍历风格输出一致 */
    sec("T5.1 下标 vs 指针（同一结果）");
    printf("下标版: ");
    for (int i = 0; i < n; i++) printf("%d ", g[i]);
    printf("\n指针版: ");
    for (int *p = g; p < g + n; p++) printf("%d ", *p);
    printf("\n");

    /* T5.2 逐元素地址：+1 跳 sizeof(int) */
    sec("T5.2 指针算术步长");
    for (int i = 0; i < n; i++)
        printf("  &g[%d]=%p   g+%d=%p\n",
               i, (const void *)&g[i], i, (const void *)(g + i));
    printf("  g+%d=%p   <- 尾后 one-past-the-end\n", n, (const void *)(g + n));
    printf("  (char*)(g+1)-(char*)g       = %td 字节\n", (char *)(g + 1) - (char *)g);
    printf("  (char*)(g+n)-(char*)(g+n-1) = %td 字节\n",
           (char *)(g + n) - (char *)(g + n - 1));

    /* T5.3 指针版循环逐步跟踪 */
    sec("T5.3 循环边界 g+n");
    for (int *p = g; p < g + n; p++)
        printf("  p=%p  *p=%2d  p<(g+%d)=%d\n",
               (const void *)p, *p, n, p < g + n);
    {
        int *end = g + n;   /* 经 noinline 函数比较，避开 -Wtautological-compare */
        printf("  退出时 p=%p，p<(g+%d)=%d\n",
               (const void *)end, n, raw_lt(end, end));
    }

    /* T5.4 小测验 */
    sec("T5.4 小测验：p = p + 2; *p = ?");
    {
        int a[3] = {1, 2, 3};
        int *p = a;
        p = p + 2;
        printf("  a[3]={1,2,3}, p=a, p=p+2  ->  *p = %d   （即 a[2]）\n", *p);
        printf("  验证 p == &a[2] : %d\n", p == &a[2]);
        printf("  再 +1 得 &a[3] 是尾后指针：可以比较，不能解引用\n");
    }

    /* T5.5 指针相减 */
    sec("T5.5 指针相减 = 元素个数");
    {
        const int *p1 = &g[1], *p2 = &g[4];
        printf("  p2 - p1 = %td   （元素差，非字节差）\n", p2 - p1);
    }

    /* T5.6 同一数组内比较：合法 */
    sec("T5.6 同一数组内比较（合法）");
    printf("  &g[0] < &g[4]      : %d\n", &g[0] < &g[4]);
    printf("  (g+n) == &g[%d]     : %d\n", n, (g + n) == &g[n]);

    /* T5.7 不同数组比较：UB */
    sec("T5.7 不同数组的指针比较（UB，结果不可依赖）");
    {
        int x[4] = {0}, y[4] = {0};
        printf("  raw_lt(x, y) = %d\n", raw_lt(x, y));
        printf("  raw_lt(y, x) = %d\n", raw_lt(y, x));
        printf("  两者都不是「可依赖的真值」——标准未定义，别拿它做判断\n");
    }

    return 0;
}
