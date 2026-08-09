#include "memlib.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Minimal memlib stub for later real allocator work. */
static char *mem_start;
static char *mem_brk;
static char *mem_max;

void mem_init(void)
{
    size_t max = 1 << 22; /* 4 MiB toy heap */
    mem_start = malloc(max);
    if (mem_start == NULL) {
        perror("mem_init");
        exit(1);
    }
    mem_brk = mem_start;
    mem_max = mem_start + max;
}

void *mem_sbrk(intptr_t incr)
{
    char *old = mem_brk;
    if (incr < 0 || mem_brk + incr > mem_max)
        return (void *)-1;
    mem_brk += incr;
    return old;
}

void *mem_heap_lo(void) { return mem_start; }
void *mem_heap_hi(void) { return mem_brk - 1; }
size_t mem_heapsize(void) { return (size_t)(mem_brk - mem_start); }
