#include "queue.h"

void queue_init(mini_queue_t *q)
{
    if (!q)
        return;
    q->head = q->tail = q->count = 0;
}

int queue_push(mini_queue_t *q, int val)
{
    if (!q || q->count >= Q_CAP)
        return -1;
    q->buf[q->tail] = val;
    q->tail = (q->tail + 1) % Q_CAP;
    q->count++;
    return 0;
}

int queue_pop(mini_queue_t *q, int *out)
{
    if (!q || q->count == 0)
        return -1;
    if (out)
        *out = q->buf[q->head];
    q->head = (q->head + 1) % Q_CAP;
    q->count--;
    return 0;
}
