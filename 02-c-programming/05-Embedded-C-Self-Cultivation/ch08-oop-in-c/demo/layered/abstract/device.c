#include "../abstract/device.h"

int layer_write(struct layer_dev *d, const uint8_t *buf, int len)
{
    if (!d || !d->ops || !d->ops->write)
        return -1;
    return d->ops->write(d->ctx, buf, len);
}
