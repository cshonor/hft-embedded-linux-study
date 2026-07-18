#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

struct device {
    char name[16];
    int  id;
    void (*open)(struct device *dev);
    void (*close)(struct device *dev);
};

struct uart_dev {
    struct device base; /* 父类必须首成员 */
    uint32_t baudrate;
    uint8_t  parity;
};

void uart_open(struct device *dev);
void uart_close(struct device *dev);
void uart_dev_init(struct uart_dev *u, const char *name, int id, uint32_t baud);

#endif
