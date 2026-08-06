#include "uart.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct uart_dev {
    uint32_t baud;
};

static void uart_hw_init(uint32_t baud)
{
    LOG_INFO("hw init baud=%u", baud);
}

err_t uart_open(uart_dev_t **out, uint32_t baud)
{
    if (!out)
        return ERR_INVAL;
    uart_dev_t *d = calloc(1, sizeof(*d));
    if (!d)
        return ERR_NOMEM;
    d->baud = baud;
    uart_hw_init(baud);
    *out = d;
    return ERR_OK;
}

void uart_close(uart_dev_t *dev)
{
    free(dev);
}

err_t uart_write(uart_dev_t *dev, const uint8_t *buf, int len)
{
    if (!dev || !buf || len <= 0)
        return ERR_INVAL;
    printf("[uart] tx %.*s\n", len, (const char *)buf);
    return ERR_OK;
}
