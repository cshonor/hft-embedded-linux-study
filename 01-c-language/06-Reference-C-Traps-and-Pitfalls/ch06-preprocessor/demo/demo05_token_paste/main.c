#include <stdio.h>

#define CAT(a, b) a##b

int main(void)
{
    int val1 = 10;
    int val2 = 20;

    printf("CAT(val,1)=%d CAT(val,2)=%d\n", CAT(val, 1), CAT(val, 2));

    return 0;
}
