#ifndef MOD_UART_H
#define MOD_UART_H

#include <stdint.h>
#include "err.h"

typedef struct uart_dev uart_dev_t;

err_t uart_open(uart_dev_t **out, uint32_t baud);
void  uart_close(uart_dev_t *dev);
err_t uart_write(uart_dev_t *dev, const uint8_t *buf, int len);

#endif
