#include "device.h"
#include <stdio.h>
#include <string.h>

void uart_open(struct device *dev)
{
    struct uart_dev *u = (struct uart_dev *)dev;
    printf("[uart] open %s id=%d baud=%u parity=%u\n",
           dev->name, dev->id, u->baudrate, u->parity);
}

void uart_close(struct device *dev)
{
    printf("[uart] close %s\n", dev->name);
}

void uart_dev_init(struct uart_dev *u, const char *name, int id, uint32_t baud)
{
    memset(u, 0, sizeof(*u));
    strncpy(u->base.name, name, sizeof(u->base.name) - 1);
    u->base.id = id;
    u->base.open = uart_open;
    u->base.close = uart_close;
    u->baudrate = baud;
    u->parity = 0;
}
