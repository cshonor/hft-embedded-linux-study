#include <stdio.h>

int main(void)
{
    int i = 3;
    i = i++ + ++i; /* UB：同一表达式多次修改 i */
    printf("Christmas UB: i = %d (编译器/优化等级不同结果不同)\n", i);
    return 0;
}
