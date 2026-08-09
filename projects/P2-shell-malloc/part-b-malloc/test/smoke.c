#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    void *p = mymalloc(32);
    assert(p != NULL);
    memset(p, 0xAB, 32);

    void *q = mycalloc(4, 8);
    assert(q != NULL);

    p = myrealloc(p, 64);
    assert(p != NULL);

    myfree(p);
    myfree(q);

    puts("part-b-malloc: basic smoke OK (stub wraps libc)");
    return 0;
}
