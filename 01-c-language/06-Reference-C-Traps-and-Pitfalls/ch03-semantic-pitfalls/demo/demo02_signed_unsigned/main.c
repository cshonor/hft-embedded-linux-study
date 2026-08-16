#include <stdio.h>

int main(void)
{
    unsigned u = 10;
    int i = -1;

    printf("i=%d u=%u\n", i, u);
    printf("i < u  => %d (wrong: looks true, actually false)\n", i < u);
    printf("(unsigned)i = %u\n", (unsigned)i);
    return 0;
}
