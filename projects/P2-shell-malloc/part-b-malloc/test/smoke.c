#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    assert(mm_init() == 0);

    void *p = mymalloc(32);
    assert(p != NULL);
    memset(p, 0xAB, 32);

    void *q = mycalloc(4, 8);
    assert(q != NULL);
    unsigned char *qb = q;
    for (int i = 0; i < 32; i++)
        assert(qb[i] == 0);

    p = myrealloc(p, 64);
    assert(p != NULL);

    /* 连续分配再全部释放：测合并，避免隐式链表只涨不缩。 */
    void *blocks[32];
    for (int i = 0; i < 32; i++) {
        blocks[i] = mymalloc(24 + (size_t)i);
        assert(blocks[i] != NULL);
        memset(blocks[i], i, 8);
    }
    for (int i = 0; i < 32; i++)
        myfree(blocks[i]);

    void *again = mymalloc(256);
    assert(again != NULL);
    myfree(again);

    myfree(p);
    myfree(q);

    puts("part-b-malloc: implicit free-list OK");
    return 0;
}
