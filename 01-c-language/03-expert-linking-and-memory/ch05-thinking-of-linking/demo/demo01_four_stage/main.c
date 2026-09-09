#include <stdio.h>

int g_init = 1;
int g_bss;

int main(void)
{
    printf("g_init=%d g_bss=%d\n", g_init, g_bss);
    return 0;
}
