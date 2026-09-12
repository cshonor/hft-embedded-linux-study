/* t4_param_decay.c —— 数组作函数参数：三种写法完全等价（C11 6.7.6.3p7）
 *
 * 证明：
 *   1. `int a[10]` / `int a[]` / `int *a` 作为形参，编译器眼里毫无区别
 *   2. 函数内 sizeof 拿到的是指针宽度，不是数组大小
 *   3. 想要「保留数组大小」必须传数组指针 `int (*a)[10]`
 *   4. 传长度是 C 的传统解法；C23 的 `int a[static n]` 只加契约不加信息
 *
 * WSL: gcc -std=c11 -O0 -Wall -o t4 t4_param_decay.c && ./t4
 */
#include <stdio.h>
#include <stddef.h>

/* --- 三种写法：_Static_assert 在编译期证明它们同类型 --- */
void f_brackets_num(int a[10]) { (void)a; }
void f_brackets_empty(int a[]) { (void)a; }
void f_pointer(int *a)         { (void)a; }

/* 这行能编过，就证明 f_brackets_num 的形参类型就是 int* */
_Static_assert(_Generic(&f_brackets_num, void (*)(int *): 1, default: 0),
               "int a[10] 形参必须等价于 int *");
_Static_assert(_Generic(&f_brackets_empty, void (*)(int *): 1, default: 0),
               "int a[] 形参必须等价于 int *");

/* --- 函数内 sizeof：全部是 8 --- */
static void show_sizes(int a[10], int *b)
{
    printf("  形参 int a[10]  内 sizeof(a) = %2zu\n", sizeof a);
    printf("  形参 int *b     内 sizeof(b) = %2zu\n", sizeof b);
}

/* --- 唯一能「带回」大小的写法：数组指针 --- */
static void keeps_size(int (*a)[10])
{
    printf("  形参 int (*a)[10] 内 sizeof(*a) = %2zu  ← 大小信息还在\n", sizeof *a);
    printf("  元素访问：(*a)[3] = %d\n", (*a)[3]);
}

/* --- C99/C11 VLA 形参：多一个长度的「语义」但仍是运行时值 --- */
static void vla_param(int n, int a[n])
{
    printf("  形参 int a[n]（n=%d）内 sizeof(a) = %2zu  ← 还是指针！\n",
           n, sizeof a);
    (void)a;
}

/* --- C23 的 static 限定：只承诺「至少 n 个元素」，不改变类型 --- */
static void static_param(int a[static 10])
{
    printf("  形参 int a[static 10] 内 sizeof(a) = %2zu  ← 仍是指针\n", sizeof a);
    (void)a;
}

int main(void)
{
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    printf("======== 1. 定义域的 sizeof ========\n");
    printf("  main 里 sizeof(arr) = %zu（定义域，数组身份完整）\n\n", sizeof arr);

    printf("======== 2. 三种形参在函数内的 sizeof ========\n");
    show_sizes(arr, arr);
    printf("  注：_Static_assert 已证明 int a[10] 与 int * 是同一种形参类型\n\n");

    printf("======== 3. 数组指针形参：大小不丢 ========\n");
    keeps_size(&arr);
    printf("\n");

    printf("======== 4. VLA 形参 / static 限定 ========\n");
    vla_param(10, arr);
    static_param(arr);
    printf("\n");

    printf("======== 5. 三种传参写法对照 ========\n");
    printf("  void f(int a[10])      sizeof(a)=8   仅文档含义\n");
    printf("  void f(int a[])        sizeof(a)=8   同上\n");
    printf("  void f(int *a)         sizeof(a)=8   同上（最诚实）\n");
    printf("  void f(int a[static 10]) sizeof(a)=8 加「非空+至少 10 个」契约\n");
    printf("  void f(int a[n])（VLA） sizeof(a)=8  n 仅运行时已知\n");
    printf("  void f(int (*a)[10])   sizeof(*a)=40 唯一保留大小的写法\n");
    printf("\n  实践：传数组一律「指针 + 长度」两个参数，别指望 sizeof。\n");
    return 0;
}
