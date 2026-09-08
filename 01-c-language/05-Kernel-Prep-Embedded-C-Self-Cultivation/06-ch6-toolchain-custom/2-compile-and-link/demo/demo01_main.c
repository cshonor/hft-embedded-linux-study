#include <stdio.h>

int g_init = 42;
int g_bss;

int main(void)
{
    printf("hello ch04 four-stage compile\n");
    printf("g_init=%d g_bss=%d\n", g_init, g_bss);
    return 0;
}
