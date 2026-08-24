#ifndef MM_H
#define MM_H

#include <stddef.h>

/* 必须先调一次。smoke 测试会调；若忘了，第一次 mymalloc 也会自动 init。 */
int mm_init(void);

void *mymalloc(size_t size);
void  myfree(void *ptr);
void *myrealloc(void *ptr, size_t newsize);
void *mycalloc(size_t nmemb, size_t size);

#endif
