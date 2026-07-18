#include <stdio.h>

int main(void)
{
    int a = 10;
    long x = 0x123456789ABCL;

    printf("correct %%d: a=%d\n", a);
    printf("WRONG %%f for int (garbage): ");
    printf("%f\n", a);

    printf("correct %%ld: x=%ld\n", x);
    printf("WRONG %%d for long (truncate): x=%d\n", (int)x);

    return 0;
}
