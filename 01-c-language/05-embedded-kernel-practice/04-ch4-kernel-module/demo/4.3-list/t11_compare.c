/* T11: 对比 —— list_head 双向 vs 单链表 vs 数组
 *
 * 时间复杂度 + 实际操作次数 + cache 行为对比
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

struct list_head { struct list_head *next, *prev; };
static inline void INIT_LIST_HEAD(struct list_head *list) { list->next = list; list->prev = list; }
static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    next->prev = new; new->next = next; new->prev = prev; prev->next = new;
}
static inline void list_add_tail(struct list_head *new, struct list_head *head) { __list_add(new, head->prev, head); }

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })
#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

struct item {
    int id;
    struct list_head node;
};

struct slist_node {
    struct slist_node *next;
    int id;
};

#define N 10000

int main(void)
{
    printf("=== T11: list_head 双向 vs 单链表 vs 数组 ===\n\n");

    /* 双向链表 */
    struct list_head dll;
    INIT_LIST_HEAD(&dll);
    struct item *items = calloc(N, sizeof(struct item));
    for (int i = 0; i < N; i++) {
        items[i].id = i;
        INIT_LIST_HEAD(&items[i].node);
        list_add_tail(&items[i].node, &dll);
    }

    /* 单链表 */
    struct slist_node *sll = NULL;
    struct slist_node *snodes = calloc(N, sizeof(struct slist_node));
    for (int i = 0; i < N; i++) {
        snodes[i].id = i;
        snodes[i].next = sll;
        sll = &snodes[i];
    }

    /* 数组 */
    int *arr = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) arr[i] = i;

    /* 遍历求和, 测时间 */
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    long sum = 0;
    struct item *pos;
    list_for_each_entry(pos, &dll, node) sum += pos->id;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long dll_ns = (t1.tv_sec - t0.tv_sec) * 1000000000 + t1.tv_nsec - t0.tv_nsec;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    sum = 0;
    for (struct slist_node *p = sll; p; p = p->next) sum += p->id;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long sll_ns = (t1.tv_sec - t0.tv_sec) * 1000000000 + t1.tv_nsec - t0.tv_nsec;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    sum = 0;
    for (int i = 0; i < N; i++) sum += arr[i];
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long arr_ns = (t1.tv_sec - t0.tv_sec) * 1000000000 + t1.tv_nsec - t0.tv_nsec;

    printf("N = %d, 遍历求和:\n\n", N);
    printf("  双向链表:   %ld ns  (每节点: 读 next + container_of + 读 id)\n", dll_ns);
    printf("  单链表:     %ld ns  (每节点: 读 next + 读 id)\n", sll_ns);
    printf("  数组:       %ld ns  (连续内存, prefetcher 友好)\n", arr_ns);
    printf("\n  数组/链表 ≈ %.1fx  (cache 局部性差距)\n", (double)dll_ns / arr_ns);

    printf("\n=== 操作复杂度对比 ===\n");
    printf("| 操作          | 双向 list_head  | 单链表 slist | 数组      |\n");
    printf("|---------------|-----------------|--------------|-----------|\n");
    printf("| 头插           | O(1)            | O(1)         | O(n)      |\n");
    printf("| 尾插           | O(1) (head.prev)| O(n)         | O(1) 摊还 |\n");
    printf("| 任意位置插     | O(1) (有指针)   | O(1)         | O(n)      |\n");
    printf("| 删中间节点     | O(1) (有指针)   | O(n) 找前驱  | O(n)      |\n");
    printf("| 反向遍历       | O(1) (head.prev)| 不支持       | O(1)      |\n");
    printf("| 查找           | O(n)            | O(n)         | O(log n)  |\n");
    printf("| 内存局部性     | 差 (节点分散)    | 差           | 好        |\n");
    printf("| 每节点开销     | 16 字节         | 8 字节       | 0         |\n");

    free(items); free(snodes); free(arr);
    return 0;
}
