#include "../abstract/device.h"

extern struct layer_dev g_uart;

int app_run(void)
{
    const char *msg = "layered-arch";
    return layer_write(&g_uart, (const uint8_t *)msg, (int)sizeof("layered-arch") - 1);
}
