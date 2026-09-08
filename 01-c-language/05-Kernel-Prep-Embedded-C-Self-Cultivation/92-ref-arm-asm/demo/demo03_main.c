#include <stdio.h>

extern int add_asm(int a, int b);

int main(void)
{
    int sum = add_asm(3, 5);
    printf("add_asm(3,5) = %d\n", sum);
    return 0;
}
