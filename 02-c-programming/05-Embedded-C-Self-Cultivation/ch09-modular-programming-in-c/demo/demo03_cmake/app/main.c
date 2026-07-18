#include "uart.h"
#include "crc.h"
#include "log.h"
#include "err.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    uart_dev_t *uart = NULL;
    const char *msg = "demo03";

    if (uart_open(&uart, 115200) != ERR_OK)
        return 1;
    uart_write(uart, (const uint8_t *)msg, (int)strlen(msg));
    uart_close(uart);
    return 0;
}
