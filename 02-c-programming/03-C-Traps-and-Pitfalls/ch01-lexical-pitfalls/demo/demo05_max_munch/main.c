#include <stdio.h>

int main(void)
{
    int a = 1, b = 2;

    /* 词法: a++ + b */
    int c = a+++b;
    printf("a+++b: a=%d b=%d c=%d\n", a, b, c);

    int x = 5, y = 3;
    int z = x---y; /* x-- - y */
    printf("x---y: x=%d y=%d z=%d\n", x, y, z);

    return 0;
}
