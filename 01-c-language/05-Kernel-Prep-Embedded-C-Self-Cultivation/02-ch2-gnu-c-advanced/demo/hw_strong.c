#include <stdio.h>

/* 上层强符号：链接时覆盖 weak 默认实现 */
void hw_init(void)
{
    puts("[strong] board-specific hw_init: GPIO/UART ready");
}
