#include "../abstract/device.h"
#include <stdio.h>

static int uart_layer_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    printf("[hw/uart] %.*s\n", len, (const char *)buf);
    return len;
}

static const struct layer_ops uart_layer_ops = { .write = uart_layer_write };

struct layer_dev g_uart = {
    .name = "UART-L1",
    .ops  = &uart_layer_ops,
    .ctx  = NULL,
};
