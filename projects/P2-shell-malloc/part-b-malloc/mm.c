#include "mm.h"

#include <stdlib.h>
#include <string.h>

/* STUB: wrap libc so the project builds. Replace with your heap. */
void *mymalloc(size_t size)
{
    return malloc(size);
}

void myfree(void *ptr)
{
    free(ptr);
}

void *myrealloc(void *ptr, size_t newsize)
{
    return realloc(ptr, newsize);
}

void *mycalloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}
