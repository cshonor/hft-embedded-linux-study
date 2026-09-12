/* t5_pointer_iter.c — 指针遍历数组：核心等价 / 尾后指针 / 关系比较
 *
 * 对应笔记 8.1.3 指针与下标
 * 环境：WSL Ubuntu gcc 13.3 / clang 18.1.3
 *       另有 Compiler Explorer gcc 13.2 / clang 18.1.0 复核，结论一致；
 *       唯一差异是 T5.7 的 UB 比较方向随编译器翻转——正是该实验要说明的点
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

    /* T5.5 指针相减：下标差，类型 ptrdiff_t */
    sec("T5.5 指针相减 = 下标差（不是字节差）");
    {
        const int *p1 = &g[1], *p2 = &g[4];
        printf("  p2 - p1               = %td  ← 下标差（元素个数）\n", p2 - p1);
        printf("  (char*)p2 - (char*)p1 = %td  ← 字节差\n",
               (const char *)p2 - (const char *)p1);
        printf("  sizeof(ptrdiff_t)     = %zu 字节（有符号类型，<stddef.h>）\n",
               sizeof(ptrdiff_t));
        printf("  字节差 / sizeof(int)  = %td  ← 编译器在类型层面替你除回去\n",
               ((const char *)p2 - (const char *)p1) / (ptrdiff_t)sizeof(int));
        printf("  注：相减和 < 一样，要求二者同属一个数组（含尾后）\n");
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

    /* T5.8 思考题：元素 vs 尾后，属同一数组 → 合法且有确定结果 */
    sec("T5.8 思考题：p=arr+3, q=arr+4, p<q ?");
    {
        int arr[4] = {1, 2, 3, 4};
        int *p = arr + 3;    /* 最后一个元素 arr[3] */
        int *q = arr + 4;    /* 尾后指针 */
        printf("  p = arr+3 = %p  （最后一个元素）\n", (const void *)p);
        printf("  q = arr+4 = %p  （尾后 one-past-the-end）\n", (const void *)q);
        printf("  p < q   = %d     ← 合法，结果由标准保证为 1\n", p < q);
        printf("  q - p   = %td     ← 同数组，允许相减\n", q - p);
        printf("  *q 不允许：尾后指针不能被解引用（§6.5.6p8）\n");
        printf("  对照 raw_lt(q,q) = %d（同一个指针，比较必相等）\n", raw_lt(q, q));
    }

    return 0;
}
