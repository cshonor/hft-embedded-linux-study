/* TLPI Ch7 练习 1 参考实现：用 sbrk + 空闲链表做 my_malloc / my_free
 * 编译: gcc -O2 -Wall -Wextra -o ex7_1 ex7_1_simple_malloc.c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

typedef struct block {
    size_t size;                /* 数据区大小（字节） */
    int free;                   /* 是否空闲 */
    struct block *next;         /* 下一个块（含已分配的，方便遍历） */
} block_t;

#define ALIGN       16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~((size_t) ALIGN - 1))
#define META        ALIGN_UP(sizeof(block_t))   /* 头部也要对齐到 16 */

static block_t *g_head;

void *my_malloc(size_t size)
{
    size = ALIGN_UP(size);
    if (size == 0) size = ALIGN;

    /* 第一遍：在链表里找一块够大且空闲的 */
    for (block_t *b = g_head; b != NULL; b = b->next) {
        if (!b->free || b->size < size) continue;

        /* 剩得够多就切开，后半段变成新的空闲块 */
        if (b->size >= size + META + ALIGN) {
            block_t *split = (block_t *) ((char *) b + META + size);
            split->size = b->size - size - META;
            split->free = 1;
            split->next = b->next;
            b->next = split;
            b->size = size;
        }
        b->free = 0;
        return (char *) b + META;
    }

    /* 第二遍：没有合适的，直接向内核要 */
    block_t *b = (block_t *) sbrk(0);
    if (sbrk((intptr_t) (META + size)) == (void *) -1)
        return NULL;

    b->size = size;
    b->free = 0;
    b->next = g_head;           /* 头插，省事 */
    g_head  = b;
    return (char *) b + META;
}

void my_free(void *ptr)
{
    if (ptr == NULL) return;

    block_t *b = (block_t *) ((char *) ptr - META);
    b->free = 1;

    /* 合并「当前块 + 紧邻的下一块」这一种情况，够用了 */
    if (b->next != NULL && b->next->free
        && (char *) b + META + b->size == (char *) b->next) {
        b->size += META + b->next->size;
        b->next = b->next->next;
    }
}

int main(void)
{
    char *a = my_malloc(100);
    char *b = my_malloc(200);
    char *c = my_malloc(100);

    printf("a=%p  b=%p  c=%p\n", (void *) a, (void *) b, (void *) c);
    printf("b - a = %ld   (期望 32 + 112 = 144)\n", (long) (b - a));
    printf("c - b = %ld   (期望 32 + 208 = 240)\n", (long) (c - b));

    for (int i = 0; i < 100; i++) a[i] = 'a';
    for (int i = 0; i < 200; i++) b[i] = 'b';
    for (int i = 0; i < 100; i++) c[i] = 'c';

    my_free(b);                     /* 中间那块先还 */
    char *d = my_malloc(200);       /* 应该原地复用 b */
    printf("my_free(b) 后 my_malloc(200) = %p  （复用 b = %s）\n",
           (void *) d, d == b ? "是" : "否");

    printf("邻居没被踩坏: a[0]=%c a[99]=%c  c[0]=%c c[99]=%c  d[0]=%c\n",
           a[0], a[99], c[0], c[99], d[0]);

    my_free(a); my_free(c); my_free(d);
    printf("收尾 sbrk(0) = %p\n", sbrk(0));
    return 0;
}
