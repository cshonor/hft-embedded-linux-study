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

    /* 隔一个释放：空闲链上有洞，下一次同尺寸 malloc 应复用，而不是只往堆顶涨。 */
    void *slot[16];
    for (int i = 0; i < 16; i++) {
        slot[i] = mymalloc(48);
        assert(slot[i] != NULL);
    }
    for (int i = 0; i < 16; i += 2)
        myfree(slot[i]);
    void *reuse = mymalloc(48);
    assert(reuse != NULL);
    int hit = 0;
    for (int i = 0; i < 16; i += 2) {
        if (reuse == slot[i])
            hit = 1;
    }
    assert(hit);
    myfree(reuse);
    for (int i = 1; i < 16; i += 2)
        myfree(slot[i]);

    myfree(p);
    myfree(q);

    puts("part-b-malloc: explicit free-list OK");
    return 0;
}
