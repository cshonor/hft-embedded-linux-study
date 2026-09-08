/* T5: hlist (哈希链表) vs list —— pprev 的设计妙用
 *
 * list_head:  双向链表, 头节点是完整结构 (next + prev), 占 16 字节
 * hlist:      哈希链表, 头节点只有 1 个指针 (first), 占 8 字节
 *             节点用 pprev (指向前一个节点的 next 字段的地址) 而非 prev
 *
 * pprev 的妙用: 删除节点时不需要判断是不是头节点
 *   *pprev = next  直接修改前驱的 next, 无论前驱是 head 还是 node
 *
 * 代价: 失去 O(1) 访问尾节点的能力
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next;
    struct hlist_node **pprev;  /* 指向前驱的 next 指针的地址 */
};

static inline void INIT_HLIST_HEAD(struct hlist_head *h) {
    h->first = NULL;
}

static inline void INIT_HLIST_NODE(struct hlist_node *h) {
    h->next = NULL;
    h->pprev = NULL;
}

static inline int hlist_empty(const struct hlist_head *h) {
    return !h->first;
}

static inline int hlist_unhashed(const struct hlist_node *h) {
    return !h->pprev;
}

static inline void __hlist_add(struct hlist_node *new,
                               struct hlist_head *h,
                               struct hlist_node *prev,
                               struct hlist_node *next) {
    /* new 的 pprev 指向 "指向 new 的那个 next 字段" */
    new->next = next;
    new->pprev = &prev->next;  /* 如果 prev 是 head 的虚拟节点... */
    /* 简化: 直接用 hlist_add_head 的逻辑 */
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h) {
    struct hlist_node *first = h->first;
    n->next = first;
    if (first)
        first->pprev = &n->next;
    h->first = n;
    n->pprev = &h->first;  /* pprev 指向 head.first 的地址 */
}

static inline void hlist_del(struct hlist_node *n) {
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;
    /* *pprev 就是 "指向 n 的那个字段", 改成 next */
    *pprev = next;
    if (next)
        next->pprev = pprev;
    /* 投毒 */
    n->next = (void *)0x100;  /* LIST_POISON1 */
    n->pprev = (void *)0x122; /* LIST_POISON2 */
}

static inline void hlist_del_init(struct hlist_node *n) {
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;
    *pprev = next;
    if (next)
        next->pprev = pprev;
    INIT_HLIST_NODE(n);
}

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })
#define hlist_entry(ptr, type, member) container_of(ptr, type, member)

/* 内核的 hlist_entry_safe: 先判 NULL 再 container_of,
 * 否则 container_of(NULL) 返回非 NULL 地址 (NULL - offset = 负数), 循环不终止 */
#define hlist_entry_safe(ptr, type, member) ({ \
    __typeof__(ptr) ____ptr = (ptr); \
    ____ptr ? hlist_entry(____ptr, type, member) : NULL; })

#define hlist_for_each(pos, head) \
    for (pos = (head)->first; pos; pos = pos->next)

#define hlist_for_each_entry(pos, head, member) \
    for (pos = hlist_entry_safe((head)->first, __typeof__(*pos), member); \
         pos; \
         pos = hlist_entry_safe((pos)->member.next, __typeof__(*pos), member))

struct item {
    int id;
    struct hlist_node hash_node;
};

int main(void)
{
    printf("=== T5: hlist (哈希链表) vs list ===\n\n");

    printf("sizeof(struct hlist_head) = %zu (只有 1 个指针)\n", sizeof(struct hlist_head));
    printf("sizeof(struct hlist_node) = %zu (next + pprev)\n", sizeof(struct hlist_node));
    printf("  -> 哈希表 N 个桶, 头节点省 N*8 字节\n\n");

    struct hlist_head buckets[4];
    for (int i = 0; i < 4; i++)
        INIT_HLIST_HEAD(&buckets[i]);

    struct item items[8];
    for (int i = 0; i < 8; i++) {
        items[i].id = i;
        INIT_HLIST_NODE(&items[i].hash_node);
        int bucket = i % 4;
        hlist_add_head(&items[i].hash_node, &buckets[bucket]);
    }

    printf("--- 各桶内容 (头插, 逆序) ---\n");
    for (int b = 0; b < 4; b++) {
        printf("  bucket[%d]: ", b);
        struct item *ip;
        hlist_for_each_entry(ip, &buckets[b], hash_node)
            printf("id=%d ", ip->id);
        printf("\n");
    }

    printf("\n--- pprev 的妙用: 验证 ---\n");
    struct hlist_node *first_node = buckets[0].first;
    printf("  bucket[0].first      = %p\n", (void*)first_node);
    printf("  first_node->pprev     = %p\n", (void*)first_node->pprev);
    printf("  &buckets[0].first     = %p (== pprev? %s)\n",
           (void*)&buckets[0].first,
           first_node->pprev == &buckets[0].first ? "yes" : "NO");
    printf("  -> pprev 指向 head.first 的地址, 删除时 *pprev=next 直接改 head.first\n");

    printf("\n--- 删 bucket[0] 的第一个节点 ---\n");
    printf("  删前: first=%p\n", (void*)buckets[0].first);
    hlist_del(first_node);
    printf("  删后: first=%p (被 *pprev=next 自动改写)\n", (void*)buckets[0].first);
    printf("  -> 不需要 if (n == head->first) 判断!\n");

    printf("\n  bucket[0] 剩余: ");
    struct item *ip;
    hlist_for_each_entry(ip, &buckets[0], hash_node)
        printf("id=%d ", ip->id);
    printf("\n");

    printf("\n=== list vs hlist 对比 ===\n");
    printf("| 维度        | list_head          | hlist             |\n");
    printf("|-------------|--------------------|-------------------|\n");
    printf("| 头节点大小  | 16 字节 (next+prev)| 8 字节 (first)    |\n");
    printf("| 节点大小    | 16 字节            | 16 字节 (next+pprev) |\n");
    printf("| 删头节点   | 需特判 (改 head)   | 不需特判 (*pprev) |\n");
    printf("| 访问尾节点 | O(1) (head->prev)  | O(n)              |\n");
    printf("| 典型用途   | 通用双向链表       | 哈希表桶          |\n");
    return 0;
}
