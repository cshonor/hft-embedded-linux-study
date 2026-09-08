#include <stdio.h>

static int g_calls;

static int side_effect(void)
{
    g_calls++;
    printf("  side_effect() called, g_calls=%d\n", g_calls);
    return 1;
}

static int never(void)
{
    printf("  never() should NOT run\n");
    return 0;
}

int main(void)
{
    g_calls = 0;
    printf("&& short-circuit:\n");
    if (0 && side_effect())
        ;
    printf("  after false &&: g_calls=%d\n\n", g_calls);

    g_calls = 0;
    printf("|| short-circuit:\n");
    if (1 || never())
        ;
    printf("  after true ||: g_calls unchanged\n\n");

    printf("NULL guard:\n");
    int *ptr = NULL;
    if (ptr != NULL && *ptr > 0)
        printf("unreachable\n");
    else
        printf("  ptr==NULL, right side of && not evaluated → no crash\n");

    return 0;
}
