#include <stdio.h>

static void tag(int a, int b)
{
    printf("  args evaluated: a=%d b=%d\n", a, b);
}

static void demo_comma(void)
{
    int a = (1, 2, 3);
    printf("comma operator: a = (1,2,3) -> %d\n", a);
}

static void demo_func_ub(void)
{
    int i = 0;
    /* 实参列表逗号不是逗号运算符，求值顺序未指定 -> UB */
    tag(i++, ++i);
    printf("after tag(i++,++i): i=%d (UB)\n", i);
}

int main(void)
{
    demo_comma();
    demo_func_ub();
    return 0;
}
