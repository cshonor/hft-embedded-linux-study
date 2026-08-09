#ifndef MEMLIB_H
#define MEMLIB_H

#include <stddef.h>
#include <stdint.h>

void  mem_init(void);
void *mem_sbrk(intptr_t incr);
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);

#endif
