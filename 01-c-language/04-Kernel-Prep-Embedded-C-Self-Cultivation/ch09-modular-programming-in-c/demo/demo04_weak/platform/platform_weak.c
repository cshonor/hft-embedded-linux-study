#include "platform.h"

/* weak 默认：仿真/WSL 空实现 */
__attribute__((weak)) uint32_t platform_tick_ms(void)
{
    return 0;
}
