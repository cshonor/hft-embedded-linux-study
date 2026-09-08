#include "dev_ops.h"
#include <stdio.h>
#include <stdlib.h>

int dev_open(struct dev_obj *d)
{
    if (!d || !d->ops || !d->ops->open)
        return -1;
    return d->ops->open(d->priv);
}

int dev_write(struct dev_obj *d, const uint8_t *buf, int len)
{
    if (!d || !d->ops || !d->ops->write)
        return -1;
    return d->ops->write(d->priv, buf, len);
}

int dev_close(struct dev_obj *d)
{
    if (!d || !d->ops || !d->ops->close)
        return -1;
    return d->ops->close(d->priv);
}

typedef struct {
    int fd;
} uart_priv_t;

typedef struct {
    int cs_pin;
} spi_priv_t;

static int uart_open(void *priv)
{
    uart_priv_t *u = priv;
    u->fd = 1;
    puts("[uart_ops] open");
    return 0;
}

static int uart_write(void *priv, const uint8_t *buf, int len)
{
    (void)priv;
    printf("[uart_ops] write %.*s\n", len, (const char *)buf);
    return len;
}

static int uart_close(void *priv)
{
    (void)priv;
    puts("[uart_ops] close");
    return 0;
}

static int spi_open(void *priv)
{
    spi_priv_t *s = priv;
    s->cs_pin = 5;
    puts("[spi_ops] open");
    return 0;
}

static int spi_write(void *priv, const uint8_t *buf, int len)
{
    spi_priv_t *s = priv;
    printf("[spi_ops] cs=%d write %.*s\n", s->cs_pin, len, (const char *)buf);
    return len;
}

static int spi_close(void *priv)
{
    (void)priv;
    puts("[spi_ops] close");
    return 0;
}

const struct dev_ops uart_ops = {
    .open  = uart_open,
    .write = uart_write,
    .close = uart_close,
};

const struct dev_ops spi_ops = {
    .open  = spi_open,
    .write = spi_write,
    .close = spi_close,
};

struct dev_obj *uart_obj_new(void)
{
    struct dev_obj *d = calloc(1, sizeof(*d));
    uart_priv_t *p = calloc(1, sizeof(*p));
    if (!d || !p) {
        free(d);
        free(p);
        return NULL;
    }
    d->name = "UART";
    d->ops = &uart_ops;
    d->priv = p;
    return d;
}

struct dev_obj *spi_obj_new(void)
{
    struct dev_obj *d = calloc(1, sizeof(*d));
    spi_priv_t *p = calloc(1, sizeof(*p));
    if (!d || !p) {
        free(d);
        free(p);
        return NULL;
    }
    d->name = "SPI";
    d->ops = &spi_ops;
    d->priv = p;
    return d;
}

void dev_obj_del(struct dev_obj *d)
{
    if (!d)
        return;
    free(d->priv);
    free(d);
}
