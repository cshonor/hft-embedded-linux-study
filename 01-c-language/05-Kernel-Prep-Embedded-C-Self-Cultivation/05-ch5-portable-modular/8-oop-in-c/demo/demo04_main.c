#include "dev_ops.h"
#include <stdio.h>

/* demo04：与 demo03 相同生命周期 API，专用于 valgrind 检测 */
int main(void)
{
    struct dev_obj *d = uart_obj_new();
    if (!d)
        return 1;

    dev_open(d);
    dev_write(d, (const uint8_t *)"lifecycle-test", 14);
    dev_close(d);

    dev_obj_del(d);
    d = NULL;
    puts("valgrind --leak-check=full ./demo04_lifecycle");
    return 0;
}
