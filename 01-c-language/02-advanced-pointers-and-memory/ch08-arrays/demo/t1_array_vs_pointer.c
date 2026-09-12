/* t1_array_vs_pointer.c —— 数组 vs 指针：逐项对照（8.1.5 实测底稿）
 *
 * 编译期类型探针用 _Generic（C11），运行期证据用 sizeof / 步长 / 符号表。
 * 目的：证明「数组不是指针」不是修辞，而是在类型系统里可验证的事实。
 *
 * WSL: gcc -std=c11 -O0 -Wall -o t1 t1_array_vs_pointer.c && ./t1
 */
#include <stdio.h>
#include <stddef.h>

static int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

/* 编译期类型探针：_Generic 在编译期选中分支，字符串就是「类型名」 */
#define TYPENAME(x) _Generic((x),          \
    int *:            "int *",             \
    int (*)[10]:      "int (*)[10]",       \
    int **:           "int **",            \
    int [10]:         "int [10]",          \
    default:          "other")

#define P(label, expr) printf("  %-16s -> %s\n", label, TYPENAME(expr))

int main(void)
{
    int *p = a;          /* 指针变量：自己有存储单元，存着一个地址 */
    int (*pa)[10] = &a;  /* 数组指针：指向整个 10 元数组 */

    printf("======== 1. 编译期类型（_Generic 探针）========\n");
    P("a", a);           /* 退化 */
    P("&a", &a);         /* 不退化 */
    P("p", p);           /* 本来就是指针 */
    P("&p", &p);         /* 指向指针的指针 */
    P("*&a", *&a);       /* *&a 又变回数组类型表达式 */
    printf("  注：a 在表达式里已退化，所以探针看到的是 int *\n");
    printf("      C 里没有「数组类型的右值」，_Generic 拿不到 int[10]\n\n");

    printf("======== 2. sizeof：对象大小 vs 变量大小 ========\n");
    printf("  sizeof(a)        = %2zu   (10 * 4，整个数组)\n", sizeof a);
    printf("  sizeof(p)        = %2zu   (指针宽度)\n", sizeof p);
    printf("  sizeof(*a)       = %2zu   (首元素 int)\n", sizeof *a);
    printf("  sizeof(&a)       = %2zu   (&a 是个指针值，不是数组)\n", sizeof &a);
    printf("  sizeof(*&a)      = %2zu   (*&a 是数组，回到 40)\n", sizeof *&a);
    printf("  sizeof(pa)       = %2zu   (数组指针也是指针)\n", sizeof pa);
    printf("  sizeof(*pa)      = %2zu   (解引用得数组)\n\n", sizeof *pa);

    printf("======== 3. 步长：类型决定一跳多远 ========\n");
    printf("  (char*)(a+1)    - (char*)a    = %2td   (int*，跳一个 int)\n",
           (char *)(a + 1) - (char *)a);
    printf("  (char*)(&a+1)   - (char*)a    = %2td   (int(*)[10]，跳整个数组)\n",
           (char *)(&a + 1) - (char *)a);
    printf("  (char*)(p+1)    - (char*)p    = %2td   (int*，同 a+1)\n",
           (char *)(p + 1) - (char *)p);
    printf("  (char*)(pa+1)   - (char*)pa   = %2td   (同 &a+1)\n",
           (char *)(pa + 1) - (char *)pa);
    printf("  注：数值上 &a == a == &a[0]，但步长 40 vs 4 天差地别\n\n");

    printf("======== 4. 可修改性：数组名不是可修改左值 ========\n");
    printf("  a[0] = 99;   合法（元素是左值）\n");
    printf("  *(a+1) = 98; 合法（同上）\n");
    printf("  p = a + 5;   合法（指针变量可改）\n");
    printf("  p++;         合法\n");
    printf("  a = p;       ✗ 编译错误：array type 'int[10]' is not assignable\n");
    printf("  a++;         ✗ 编译错误：lvalue required as increment operand\n");
    printf("  &a = ...;    ✗ 编译错误：lvalue required as left operand\n");
    a[0] = 99; *(a + 1) = 98; p = a + 5; p++;
    printf("  a[0]=%d a[1]=%d p 现在指向 a[5]=%d\n\n", a[0], a[1], *p);

    printf("======== 5. 传参：唯一看起来「等价」的场合 ========\n");
    printf("  void f(int x[10])  <=>  void f(int *x)\n");
    printf("  函数内 sizeof(x) 永远是 %zu（指针），数组大小信息丢失\n\n", sizeof(void *));

    printf("======== 6. 符号表：400 字节 vs 8 字节 ========\n");
    printf("  nm --print-size 见 run.sh 输出\n");
    return 0;
}
