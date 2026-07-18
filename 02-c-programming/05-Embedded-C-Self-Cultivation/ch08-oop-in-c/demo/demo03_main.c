#include "dev_ops.h"
#include <string.h>

static void app_send(struct dev_obj *dev, const char *msg)
{
    dev_open(dev);
    dev_write(dev, (const uint8_t *)msg, (int)strlen(msg));
    dev_close(dev);
}

int main(void)
{
    struct dev_obj *uart = uart_obj_new();
    struct dev_obj *spi  = spi_obj_new();

    app_send(uart, "hello-uart");
    app_send(spi,  "hello-spi");

    dev_obj_del(uart);
    dev_obj_del(spi);
    return 0;
}
