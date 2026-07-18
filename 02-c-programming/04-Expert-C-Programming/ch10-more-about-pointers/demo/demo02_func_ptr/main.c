#include <stdio.h>

typedef int (*MathFunc)(int, int);

static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }

/* 命令表 / 中断向量风格：函数指针数组 */
static MathFunc ops[] = { add, sub };

static void dispatch(int idx, int x, int y)
{
    if (idx >= 0 && idx < 2)
        printf("ops[%d](%d,%d)=%d\n", idx, x, y, ops[idx](x, y));
}

int main(void)
{
    MathFunc cb = add;
    printf("callback: cb(3,5)=%d\n", cb(3, 5));
    printf("via name: add(3,5)=%d\n", add(3, 5));

    dispatch(0, 10, 3);
    dispatch(1, 10, 3);
    return 0;
}
