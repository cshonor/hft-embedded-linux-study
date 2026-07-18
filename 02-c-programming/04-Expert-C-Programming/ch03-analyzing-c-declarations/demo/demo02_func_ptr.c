#include <stdio.h>

typedef int (*MathOp)(int, int);

static int add(int a, int b) { return a + b; }

int main(void)
{
    MathOp op = add;
    printf("op(2,3)=%d\n", op(2, 3));
    return 0;
}
