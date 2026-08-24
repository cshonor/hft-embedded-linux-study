#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* SPSC：capacity 必须是 2 的幂；实际最多存 capacity-1 个，用来区分满/空。 */
struct ringbuf {
    uint8_t *buffer;
    size_t capacity;
    size_t elem_size;
    _Atomic(size_t) head __attribute__((aligned(64)));
    char _pad1[64 - sizeof(_Atomic(size_t))];
    _Atomic(size_t) tail __attribute__((aligned(64)));
};

static inline int ringbuf_init(struct ringbuf *rb, size_t capacity, size_t elem_size)
{
    if (capacity < 2 || (capacity & (capacity - 1)) != 0)
        return -1;
    rb->buffer = malloc(capacity * elem_size);
    if (!rb->buffer)
        return -1;
    rb->capacity = capacity;
    rb->elem_size = elem_size;
    atomic_store(&rb->head, 0);
    atomic_store(&rb->tail, 0);
    return 0;
}

static inline void ringbuf_destroy(struct ringbuf *rb)
{
    free(rb->buffer);
    rb->buffer = NULL;
}

static inline int ringbuf_push(struct ringbuf *rb, const void *data)
{
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t next = (head + 1) & (rb->capacity - 1);
    if (next == atomic_load_explicit(&rb->tail, memory_order_acquire))
        return -1;
    memcpy(rb->buffer + head * rb->elem_size, data, rb->elem_size);
    atomic_store_explicit(&rb->head, next, memory_order_release);
    return 0;
}

static inline int ringbuf_pop(struct ringbuf *rb, void *data)
{
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    if (tail == atomic_load_explicit(&rb->head, memory_order_acquire))
        return -1;
    memcpy(data, rb->buffer + tail * rb->elem_size, rb->elem_size);
    atomic_store_explicit(&rb->tail, (tail + 1) & (rb->capacity - 1), memory_order_release);
    return 0;
}

#endif
