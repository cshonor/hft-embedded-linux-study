#include "sensor.h"

static sensor_cb_t s_cb;
static void *s_ctx;

void sensor_register_cb(sensor_cb_t cb, void *ctx)
{
    s_cb = cb;
    s_ctx = ctx;
}

void sensor_poll(void)
{
    if (s_cb)
        s_cb(42, s_ctx);
}
