#include "../platform/platform.h"

/* 强符号板级实现：链接时覆盖 weak */
uint32_t platform_tick_ms(void)
{
    return 1234;
}
