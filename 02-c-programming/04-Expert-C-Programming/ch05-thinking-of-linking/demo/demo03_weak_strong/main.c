#include <stdio.h>

extern int g_val;

int main(void)
{
    printf("g_val=%d (weak + strong -> 20)\n", g_val);
    return 0;
}
