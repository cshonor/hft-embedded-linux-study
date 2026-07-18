#include "counter.h"
#include <stdlib.h>

struct counter {
    int value;
};

counter_t *counter_create(int initial)
{
    counter_t *c = malloc(sizeof(*c));
    if (c)
        c->value = initial;
    return c;
}

void counter_destroy(counter_t *c)
{
    free(c);
}

int counter_get(const counter_t *c)
{
    return c ? c->value : 0;
}

void counter_inc(counter_t *c)
{
    if (c)
        c->value++;
}
