#ifndef MINI_QUEUE_H
#define MINI_QUEUE_H

#include <stdint.h>
#include <stddef.h>

#define Q_CAP 8

typedef struct {
    int  buf[Q_CAP];
    int  head;
    int  tail;
    int  count;
} mini_queue_t;

void queue_init(mini_queue_t *q);
int  queue_push(mini_queue_t *q, int val);
int  queue_pop(mini_queue_t *q, int *out);

#endif
