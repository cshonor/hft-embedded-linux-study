#include "../sensor/sensor.h"
#include "../../common/log.h"
#include <stdio.h>

static void on_sample(int value, void *ctx)
{
    const char *tag = ctx ? (const char *)ctx : "?";
    LOG_INFO("%s got sample=%d", tag, value);
}

int main(void)
{
    sensor_register_cb(on_sample, (void *)"app");
    sensor_poll();
    puts("sensor.h 不 include app — 循环依赖由回调解除");
    return 0;
}
