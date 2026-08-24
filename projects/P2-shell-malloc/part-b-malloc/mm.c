#include "mm.h"
#include "memlib.h"

#include <stdint.h>
#include <string.h>

/*
 * 隐式空闲链表（CSAPP 风格）。
 *
 * 每个块：header(size|alloc) + payload + footer(size|alloc)
 * 最低 3 bit 当标志：bit0=1 已分配。size 含头尾，8 字节对齐。
 *
 * 新手为什么要 footer：free 时看「前一块」是否空闲，才能合并，避免堆碎成芝麻。
 */

#define WSIZE 8
#define DSIZE 16
#define CHUNKSIZE (1 << 12)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~(size_t)0x7)
#define GET_ALLOC(p) (GET(p) & (size_t)0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;
static int inited;

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        return bp;
    }
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    char *bp = mem_sbrk((intptr_t)size);
    if (bp == (void *)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

static void *find_fit(size_t asize)
{
    for (char *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if (csize - asize >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

int mm_init(void)
{
    mem_init();
    heap_listp = mem_sbrk(4 * WSIZE);
    if (heap_listp == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    inited = 1;
    return 0;
}

static int ensure_init(void)
{
    if (inited)
        return 0;
    return mm_init();
}

void *mymalloc(size_t size)
{
    if (ensure_init() < 0 || size == 0)
        return NULL;

    size_t asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    void *bp = find_fit(asize);
    if (bp == NULL) {
        size_t extend = MAX(asize, CHUNKSIZE);
        bp = extend_heap(extend / WSIZE);
        if (bp == NULL)
            return NULL;
    }
    place(bp, asize);
    return bp;
}

void myfree(void *ptr)
{
    if (ptr == NULL)
        return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

void *myrealloc(void *ptr, size_t newsize)
{
    if (ptr == NULL)
        return mymalloc(newsize);
    if (newsize == 0) {
        myfree(ptr);
        return NULL;
    }
    void *np = mymalloc(newsize);
    if (np == NULL)
        return NULL;
    size_t old = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (newsize < old)
        old = newsize;
    memcpy(np, ptr, old);
    myfree(ptr);
    return np;
}

void *mycalloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *p = mymalloc(bytes);
    if (p)
        memset(p, 0, bytes);
    return p;
}
