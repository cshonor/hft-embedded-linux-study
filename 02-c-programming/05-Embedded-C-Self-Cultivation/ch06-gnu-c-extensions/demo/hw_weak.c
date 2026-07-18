#include <stdio.h>

/* 底层默认弱实现：无上层覆盖时使用 */
void hw_init(void) __attribute__((weak));

void hw_init(void)
{
    puts("[weak] default hw_init: noop stub");
}
