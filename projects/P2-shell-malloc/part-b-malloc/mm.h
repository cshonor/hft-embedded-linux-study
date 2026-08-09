#ifndef MM_H
#define MM_H

#include <stddef.h>

void *mymalloc(size_t size);
void  myfree(void *ptr);
void *myrealloc(void *ptr, size_t newsize);
void *mycalloc(size_t nmemb, size_t size);

#endif
