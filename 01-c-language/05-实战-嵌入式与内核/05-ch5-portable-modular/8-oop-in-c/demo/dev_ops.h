#ifndef DEV_OPS_H
#define DEV_OPS_H

#include <stdint.h>

struct dev_ops {
    int (*open)(void *dev);
    int (*write)(void *dev, const uint8_t *buf, int len);
    int (*close)(void *dev);
};

struct dev_obj {
    const char *name;
    const struct dev_ops *ops;
    void *priv;
};

int dev_open(struct dev_obj *d);
int dev_write(struct dev_obj *d, const uint8_t *buf, int len);
int dev_close(struct dev_obj *d);

extern const struct dev_ops uart_ops;
extern const struct dev_ops spi_ops;

struct dev_obj *uart_obj_new(void);
struct dev_obj *spi_obj_new(void);
void dev_obj_del(struct dev_obj *d);

#endif
