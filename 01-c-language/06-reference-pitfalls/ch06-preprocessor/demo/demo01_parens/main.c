#include <stdio.h>

#define SQUARE_BAD(x) x * x
#define SQUARE_GOOD(x) ((x) * (x))

int main(void)
{
    int a = 3;

    printf("a=%d\n", a);
    printf("SQUARE_BAD(a+1)  = %d (expect 7, not 16)\n", SQUARE_BAD(a + 1));
    printf("SQUARE_GOOD(a+1) = %d (expect 16)\n", SQUARE_GOOD(a + 1));

    return 0;
}
