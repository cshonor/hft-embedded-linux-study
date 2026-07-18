#include "uart.h"
#include "crc.h"
#include "log.h"
#include "err.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    uart_dev_t *uart = NULL;
    const char *msg = "demo02";

    if (uart_open(&uart, 115200) != ERR_OK) {
        LOG_ERR("uart_open failed");
        return 1;
    }

    uint8_t c = crc8_sum((const uint8_t *)msg, strlen(msg));
    LOG_INFO("crc8=0x%02x", c);
    uart_write(uart, (const uint8_t *)msg, (int)strlen(msg));
    uart_close(uart);
    return 0;
}
