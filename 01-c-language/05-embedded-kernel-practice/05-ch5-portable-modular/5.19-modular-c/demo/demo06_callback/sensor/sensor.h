#ifndef SENSOR_H
#define SENSOR_H

typedef void (*sensor_cb_t)(int value, void *ctx);

void sensor_register_cb(sensor_cb_t cb, void *ctx);
void sensor_poll(void);

#endif
