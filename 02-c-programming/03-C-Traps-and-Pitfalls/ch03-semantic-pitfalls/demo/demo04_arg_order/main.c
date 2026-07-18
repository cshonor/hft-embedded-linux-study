#include <stdio.h>

static void pair(int a, int b)
{
    printf("  pair(%d, %d)\n", a, b);
}

int main(void)
{
    int i = 1;
    printf("UB demo: func(i++, i) — run multiple times, order may vary\n");
    i = 1;
    pair(i++, i);
    return 0;
}
