#ifndef LAYER_DEVICE_H
#define LAYER_DEVICE_H

#include <stdint.h>

struct layer_ops {
    int (*write)(void *ctx, const uint8_t *buf, int len);
};

struct layer_dev {
    const char *name;
    const struct layer_ops *ops;
    void *ctx;
};

int layer_write(struct layer_dev *d, const uint8_t *buf, int len);

#endif
