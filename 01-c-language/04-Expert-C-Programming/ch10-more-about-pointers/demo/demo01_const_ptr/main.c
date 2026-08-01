#include <stdio.h>

static void test_const_ptr(void)
{
    int a = 1, b = 2;
    const int *p1 = &a;   /* 指向常量：*p1 不可改，p1 可改指向 */
    int *const p2 = &a;   /* 指针常量：p2 不可改指向，*p2 可改 */

    p1 = &b;
    *p2 = 10;
    printf("const int *p1 -> b=%d; int *const p2 -> a=%d\n", *p1, *p2);

    /* *p1 = 10; 编译错误 */
    /* p2 = &b;  编译错误 */
}

int main(void)
{
    test_const_ptr();
    return 0;
}
