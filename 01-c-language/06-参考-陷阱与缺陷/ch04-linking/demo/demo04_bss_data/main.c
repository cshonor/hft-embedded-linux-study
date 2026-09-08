#include <stdio.h>

int g_bss;
int g_data = 42;

int main(void)
{
    printf("g_bss=%d (expect 0) g_data=%d\n", g_bss, g_data);
    return 0;
}
