#include <stdio.h>
#include "device.h"

int main(void)
{
    struct uart_dev uart;
    uart_dev_init(&uart, "UART0", 1, 115200);

    /* 向上转型：首地址继承 */
    struct device *dev = (struct device *)&uart;
    printf("dev=%p uart=%p (same address)\n", (void *)dev, (void *)&uart);

    dev->open(dev);
    dev->close(dev);

    printf("gdb: print dev->open  # 函数指针地址\n");
    return 0;
}
