#include <stdio.h>

int g_normal = 1;

int g_custom __attribute__((section(".my_data"))) = 0xDEADBEEF;

const char g_flash_const[] __attribute__((section(".ro_custom")))
    = "stored in custom ro section";

void in_text(void) __attribute__((section(".my_text")));

void in_text(void)
{
    puts("function in .my_text section");
}

int main(void)
{
    printf("g_normal  = %d @ %p\n", g_normal, (void *)&g_normal);
    printf("g_custom  = 0x%x @ %p\n", g_custom, (void *)&g_custom);
    printf("g_flash   = \"%s\" @ %p\n", g_flash_const, (void *)g_flash_const);
    in_text();
    puts("readelf -S demo02_custom_section | grep my_");
    return 0;
}
