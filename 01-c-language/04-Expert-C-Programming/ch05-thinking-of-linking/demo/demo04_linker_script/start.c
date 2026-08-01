#include <stdint.h>

extern char _stext, _etext, _sdata, _edata, _sbss, _ebss;

int g_data = 42;
int g_bss;

void _start(void)
{
    (void)_stext;
    (void)_etext;
    (void)_sdata;
    (void)_edata;
    (void)_sbss;
    (void)_ebss;
    g_bss = 1;
    for (;;)
        ;
}
