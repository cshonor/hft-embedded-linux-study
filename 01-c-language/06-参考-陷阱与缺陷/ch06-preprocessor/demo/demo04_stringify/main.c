#include <stdio.h>

#define PRINT_VAR(x) printf(#x " = %d\n", x)

int main(void)
{
    int num = 42;
    int a = 1, b = 2;

    PRINT_VAR(num);
    PRINT_VAR(a + b);

    return 0;
}
