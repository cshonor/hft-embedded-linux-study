#include <stdio.h>

#define MULT(a, b) a * b
#define MULT_SAFE(a, b) ((a) * (b))

int main(void)
{
    int bad = MULT(1 + 2, 3 + 4);
    int ok  = MULT_SAFE(1 + 2, 3 + 4);

    printf("MULT(1+2,3+4)=%d (expected 11 not 21)\n", bad);
    printf("MULT_SAFE=%d\n", ok);
    return 0;
}
