#include <stdio.h>
#include "counter.h"

int main(void)
{
    counter_t *c = counter_create(10);
    if (!c)
        return 1;

    counter_inc(c);
    counter_inc(c);
    printf("counter=%d (opaque struct, no direct member access)\n", counter_get(c));

    counter_destroy(c);
    c = NULL;
    return 0;
}
