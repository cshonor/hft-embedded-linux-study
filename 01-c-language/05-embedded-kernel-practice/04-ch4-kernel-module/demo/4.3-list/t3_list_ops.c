/* T3: list_del 的 POISON 技巧 + list_del_init 的区别
 *
 * 内核 list_del 把 next/prev 指向 LIST_POISON1/2 (0x100/0x122),
 * 任何后续解引用都会立刻 page fault —— 快速暴露 use-after-del.
 *
 * list_del_init 删除后重新初始化为自指, 节点可以安全重用.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <setjmp.h>

struct list_head { struct list_head *next, *prev; };

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list; list->prev = list;
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void __list_del_entry(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}

/* 内核版: 删除后投毒 */
#define LIST_POISON1 ((void *)0x100)
#define LIST_POISON2 ((void *)0x122)

static inline void list_del(struct list_head *entry) {
    __list_del_entry(entry);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

/* list_del_init: 删除后重新初始化为自指 */
static inline void list_del_init(struct list_head *entry) {
    __list_del_entry(entry);
    INIT_LIST_HEAD(entry);
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

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

int main(void)
{
    printf("=== T3: list_del POISON vs list_del_init ===\n\n");

    struct list_head head;
    INIT_LIST_HEAD(&head);

    struct item a = { 1, {} }, b = { 2, {} }, c = { 3, {} };
    INIT_LIST_HEAD(&a.node);
    INIT_LIST_HEAD(&b.node);
    INIT_LIST_HEAD(&c.node);
    list_add_tail(&a.node, &head);
    list_add_tail(&b.node, &head);
    list_add_tail(&c.node, &head);

    printf("--- 初始链表 ---\n");
    struct item *p;
    list_for_each_entry(p, &head, node)
        printf("  id=%d  node=%p  next=%p  prev=%p\n",
               p->id, (void*)&p->node, (void*)p->node.next, (void*)p->node.prev);

    /* 删 b, 用 list_del (投毒) */
    list_del(&b.node);
    printf("\n--- 删 b (list_del 投毒) ---\n");
    printf("  b.node.next = %p (LIST_POISON1=%p? %s)\n",
           (void*)b.node.next, LIST_POISON1,
           b.node.next == LIST_POISON1 ? "yes" : "NO");
    printf("  b.node.prev = %p (LIST_POISON2=%p? %s)\n",
           (void*)b.node.prev, LIST_POISON2,
           b.node.prev == LIST_POISON2 ? "yes" : "NO");

    printf("\n  剩余链表: ");
    list_for_each_entry(p, &head, node)
        printf("id=%d ", p->id);
    printf("\n");

    /* 删 c, 用 list_del_init (自指) */
    list_del_init(&c.node);
    printf("\n--- 删 c (list_del_init 自指) ---\n");
    printf("  c.node.next = %p (== &c.node? %s)\n",
           (void*)c.node.next, c.node.next == &c.node ? "yes" : "NO");
    printf("  c.node.prev = %p (== &c.node? %s)\n",
           (void*)c.node.prev, c.node.prev == &c.node ? "yes" : "NO");
    printf("  -> c 可以安全重用 (重新 list_add 不会崩)\n");

    /* 重用 c */
    list_add_tail(&c.node, &head);
    printf("\n--- 重用 c (list_add_tail) ---\n");
    printf("  链表: ");
    list_for_each_entry(p, &head, node)
        printf("id=%d ", p->id);
    printf("\n");

    /* b 不能重用 (已投毒) */
    printf("\n--- 尝试重用 b (已投毒) 会怎样? ---\n");
    printf("  b.node.next=%p b.node.prev=%p\n", (void*)b.node.next, (void*)b.node.prev);
    printf("  如果 list_add(&b.node, &head): next->prev=new 会写 0x100+8 = 0x108 -> segfault\n");
    /* 不真的执行, 避免 segfault */
    return 0;
}
