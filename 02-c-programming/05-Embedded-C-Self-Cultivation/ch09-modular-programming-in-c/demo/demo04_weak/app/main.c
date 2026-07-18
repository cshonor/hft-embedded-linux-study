#include "../platform/platform.h"
#include "../../common/log.h"
#include <stdio.h>

int main(void)
{
    uint32_t t = platform_tick_ms();
    LOG_INFO("tick_ms=%u", t);
    printf("weak-only build: tick=0; with hw_board: tick=1234\n");
    return t == 1234 ? 0 : 0; /* 两种链接均可运行 */
}
