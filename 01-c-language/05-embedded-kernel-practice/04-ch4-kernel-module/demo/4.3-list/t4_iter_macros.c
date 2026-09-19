/* T4: 遍历宏家族 —— list_for_each vs list_for_each_entry vs list_for_each_safe
 *
 * - list_for_each:        遍历 list_head 指针, 需要手动 container_of
 * - list_for_each_entry:  直接遍历宿主指针, 内部做 container_of
 * - list_for_each_safe:   多一个临时变量, 删除当前节点不破坏遍历
 * - list_for_each_entry_reverse: 反向遍历
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list; list->prev = list;
}
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new; new->next = next; new->prev = prev; prev->next = new;
}
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev; prev->next = next;
}
static inline void __list_del_entry(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}
static inline void list_del_init(struct list_head *entry) {
    __list_del_entry(entry);
    INIT_LIST_HEAD(entry);
}
static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}
static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* 基础版: 遍历 list_head 指针 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/* entry 版: 直接遍历宿主 */
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

/* reverse 版 */
#define list_for_each_entry_reverse(pos, head, member) \
    for (pos = list_entry((head)->prev, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.prev, __typeof__(*pos), member))

/* safe 版: 删除当前节点不破坏遍历 */
#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member), \
         n = list_entry(pos->member.next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = n, n = list_entry(n->member.next, __typeof__(*n), member))

struct item {
    int id;
    struct list_head node;
};

int main(void)
{
    printf("=== T4: 遍历宏家族 ===\n\n");

    struct list_head head;
    INIT_LIST_HEAD(&head);

    struct item items[5];
    for (int i = 0; i < 5; i++) {
        items[i].id = i;
        INIT_LIST_HEAD(&items[i].node);
        list_add_tail(&items[i].node, &head);
    }

    printf("--- list_for_each (list_head 指针, 手动 container_of) ---\n");
    struct list_head *lp;
    list_for_each(lp, &head) {
        struct item *it = list_entry(lp, struct item, node);
        printf("  id=%d\n", it->id);
    }

    printf("\n--- list_for_each_entry (直接遍历宿主) ---\n");
    struct item *ip;
    list_for_each_entry(ip, &head, node) {
        printf("  id=%d\n", ip->id);
    }

    printf("\n--- list_for_each_entry_reverse (反向) ---\n");
    list_for_each_entry_reverse(ip, &head, node) {
        printf("  id=%d\n", ip->id);
    }

    printf("\n--- list_for_each_entry_safe (边遍历边删偶数 id) ---\n");
    /* safe 版: 删除当前节点不会让 pos->next 失效 */
    struct item *pos, *n;
    list_for_each_entry_safe(pos, n, &head, node) {
        printf("  visiting id=%d, next will be id=%d\n", pos->id, n->id);
        if (pos->id % 2 == 0) {
            printf("    -> deleting id=%d\n", pos->id);
            list_del_init(&pos->node);
        }
    }

    printf("\n--- 删除后剩余 ---\n");
    list_for_each_entry(ip, &head, node)
        printf("  id=%d\n", ip->id);

    printf("\n=== 关键: safe 版 vs 非 safe 版 ===\n");
    printf("  非 safe: 删 pos 后 pos->member.next 被改写, 遍历崩溃\n");
    printf("  safe:    用 n 预存下一个, 删 pos 不影响 n\n");
    return 0;
}
