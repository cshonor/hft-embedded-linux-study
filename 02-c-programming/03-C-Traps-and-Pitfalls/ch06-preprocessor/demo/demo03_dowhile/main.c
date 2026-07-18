#include <stdio.h>

#define GOOD_STEP() do { printf("step1\n"); printf("step2\n"); } while (0)
#define SWAP(a, b) do { int t = (a); (a) = (b); (b) = t; } while (0)

int main(void)
{
    int x = 1, y = 9;
    int flag = 1;

    if (flag)
        GOOD_STEP();
    else
        printf("else branch\n");

    SWAP(x, y);
    printf("after SWAP: x=%d y=%d\n", x, y);
    puts("BAD_STEP() in if/else fails to compile — see main_bad.c");

    return 0;
}
