#include "mm.h"
#include "memlib.h"

#include <stdint.h>
#include <string.h>

/*
 * 显式空闲链表（CSAPP Phase 2）。
 *
 * 隐式链表找空闲块要扫整个堆（含已分配块）。空闲块 payload 开头存 next/prev，
 * malloc 只沿着空闲链走。已分配块没有这两个指针，用户数据从 header 后开始。
 *
 * 64 位：字长 8。最小空闲块 = header + next + prev + footer = 32。
 * 最低 3 bit 当标志：bit0=1 已分配。size 含头尾，8 字节对齐。
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

#define GET_NEXT(bp) (*(void **)(bp))
#define GET_PREV(bp) (*(void **)((char *)(bp) + WSIZE))
#define SET_NEXT(bp, val) (*(void **)(bp) = (val))
#define SET_PREV(bp, val) (*(void **)((char *)(bp) + WSIZE) = (val))

static char *heap_listp;
static void *free_list_head;
static int inited;

static void remove_from_free_list(void *bp)
{
    void *next = GET_NEXT(bp);
    void *prev = GET_PREV(bp);
    if (prev)
        SET_NEXT(prev, next);
    else
        free_list_head = next;
    if (next)
        SET_PREV(next, prev);
}

static void insert_to_free_list(void *bp)
{
    SET_NEXT(bp, free_list_head);
    SET_PREV(bp, NULL);
    if (free_list_head)
        SET_PREV(free_list_head, bp);
    free_list_head = bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        insert_to_free_list(bp);
        return bp;
    }
    if (prev_alloc && !next_alloc) {
        void *nxt = NEXT_BLKP(bp);
        remove_from_free_list(nxt);
        size += GET_SIZE(HDRP(nxt));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_to_free_list(bp);
        return bp;
    }
    if (!prev_alloc && next_alloc) {
        void *prv = PREV_BLKP(bp);
        remove_from_free_list(prv);
        size += GET_SIZE(HDRP(prv));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(prv), PACK(size, 0));
        insert_to_free_list(prv);
        return prv;
    }

    void *prv = PREV_BLKP(bp);
    void *nxt = NEXT_BLKP(bp);
    remove_from_free_list(prv);
    remove_from_free_list(nxt);
    size += GET_SIZE(HDRP(prv)) + GET_SIZE(HDRP(nxt));
    PUT(HDRP(prv), PACK(size, 0));
    PUT(FTRP(nxt), PACK(size, 0));
    insert_to_free_list(prv);
    return prv;
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
    for (void *bp = free_list_head; bp != NULL; bp = GET_NEXT(bp)) {
        if (GET_SIZE(HDRP(bp)) >= asize)
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    remove_from_free_list(bp);
    if (csize - asize >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *nxt = NEXT_BLKP(bp);
        PUT(HDRP(nxt), PACK(csize - asize, 0));
        PUT(FTRP(nxt), PACK(csize - asize, 0));
        insert_to_free_list(nxt);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

int mm_init(void)
{
    mem_init();
    free_list_head = NULL;
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
