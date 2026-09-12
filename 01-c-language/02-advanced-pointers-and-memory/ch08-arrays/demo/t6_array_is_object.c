/* t6_array_is_object.c —— 正面证据：数组确实是「对象」（8.1.5 实测底稿）
 *
 * t1–t5 都在证明「数组 ≠ 指针」；这一组换方向，证明
 * 「数组是对象」不是修辞 —— 它在类型系统、对齐、存储、拷贝四层都是实体。
 *
 * WSL: gcc -std=c11 -O0 -Wall -o t6 t6_array_is_object.c && ./t6
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

struct Holder { int a[10]; int n; };
union  U      { int a[10]; long l; };

/* 编译期证据：数组类型有对齐要求（C11 §6.5.3.4p3 末句） */
_Static_assert(_Alignof(int[10])  == _Alignof(int), "数组的对齐 = 元素的对齐");
_Static_assert(_Alignof(int[100]) == _Alignof(int), "与长度无关");
_Static_assert(sizeof(int[10])    == 40,            "sizeof(type-name) 不退化");

int main(void)
{
    int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int b[10];

    puts("======== A. 类型系统里「数组类型」真实存在 ========");
    printf("  sizeof(int[10])    = %zu   <- type-name，不退化\n", sizeof(int[10]));
    printf("  _Alignof(int)      = %zu\n", _Alignof(int));
    printf("  _Alignof(int[10])  = %zu\n", _Alignof(int[10]));
    printf("  _Alignof(int[100]) = %zu   <- 数组的对齐 = 元素的对齐\n\n",
           _Alignof(int[100]));

    puts("======== B. 同一个地址，两种类型 ========");
    printf("  (void*)&a == (void*)a     : %d\n", (void *)&a == (void *)a);
    printf("  (void*)&a == (void*)&a[0] : %d\n", (void *)&a == (void *)&a[0]);
    printf("  sizeof &a                 = %zu   <- 表达式是指针值\n", sizeof &a);
    printf("  sizeof a                  = %zu   <- 表达式是数组对象\n\n", sizeof a);

    puts("======== C. 数组可作结构体成员，随整体赋值被一起拷 ========");
    {
        struct Holder h1, h2;
        int same = 1;
        for (int i = 0; i < 10; i++) h1.a[i] = i * i;
        h1.n = 10;
        h2 = h1;                        /* ✅ 结构体整体赋值 —— 里面的数组被拷 */
        for (int i = 0; i < 10; i++) if (h2.a[i] != h1.a[i]) same = 0;
        printf("  h2 = h1 后数组逐元素相同 : %d   (h2.a[9] = %d)\n", same, h2.a[9]);
        printf("  sizeof(struct Holder)   = %zu\n", sizeof(struct Holder));
        printf("  &h1.a 的类型是数组指针  : %d\n",
               _Generic(&h1.a, int (*)[10]: 1, default: 0));
    }
    puts("");

    puts("======== D. 联合体成员共享同一块存储 ========");
    {
        union U u;
        memset(&u, 0, sizeof u);
        u.a[0] = 0x41424344;
        printf("  u.a[0] = 0x41424344  ->  u.l 低 32 位 = 0x%08x\n",
               (unsigned)(u.l & 0xffffffffu));
        printf("  sizeof(union U) = %zu\n\n", sizeof(union U));
    }

    puts("======== E. 对象级整块拷贝 ========");
    memcpy(b, a, sizeof a);             /* 用 sizeof a 拿「对象」大小 */
    {
        int ok = 1;
        for (int i = 0; i < 10; i++) if (b[i] != a[i]) ok = 0;
        printf("  memcpy(b, a, sizeof a) 后相同 : %d\n", ok);
    }
    {
        char buf[sizeof a];             /* 编译期常量，可作数组维度 */
        printf("  char buf[sizeof a] 的 sizeof = %zu\n\n", sizeof buf);
    }

    puts("======== F. 反证：_Generic 拿不到 int[10] 说明什么 ========");
    printf("  _Generic(a,  int (*)[10]:...) 命中 = %d   <- 0：控制表达式已退化\n",
           _Generic(a, int (*)[10]: 1, default: 0));
    printf("  _Generic(&a, int (*)[10]:...) 命中 = %d   <- 1：&a 就是数组指针\n",
           _Generic(&a, int (*)[10]: 1, default: 0));
    printf("  结论：C 里没有「数组类型的右值」，但这不等于数组不是对象\n");
    printf("        —— 它只是不参与 lvalue conversion（§6.3.2.1p2）\n\n");

    puts("======== G. 计数惯用法 sizeof a / sizeof a[0] ========");
    printf("  定义域：sizeof a / sizeof a[0] = %zu   <- 编译期常量\n",
           sizeof a / sizeof a[0]);
    {
        int vla_n = 7;
        int vla[vla_n];
        printf("  VLA：  sizeof vla / sizeof vla[0] = %zu   <- 运行时求值\n",
               sizeof vla / sizeof vla[0]);
        printf("         VLA 的 sizeof 是运行时值，不能作常量（§6.5.3.4p2）\n");
    }
    puts("  形参里：恒为 8/4 = 2（错）—— t4_param_decay.c 第 5 组实测，8.1.6 详");

    return 0;
}
