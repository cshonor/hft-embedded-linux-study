/* T1: 侵入式链表 vs 外挂链表 —— 内存布局与分配次数对比
 *
 * 侵入式: 节点内嵌在宿主里, 插入零分配
 * 外挂式: 每个节点单独 malloc, 持有 data 指针
 *
 * 对比: sizeof / 分配次数 / cache line 行为
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* === 侵入式链表 (内核风格) === */
struct list_head {
    struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list; list->prev = list;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

/* 宿主: 设备 */
struct device {
    int              id;
    char             name[12];
    struct list_head node;   /* 侵入式: 内嵌 */
    unsigned         irq;
};

/* === 外挂链表 (传统风格) === */
struct ext_node {
    struct ext_node *next;
    struct device   *data;  /* 持有数据指针 */
};

int main(void)
{
    printf("=== T1: 侵入式 vs 外挂式 ===\n\n");

    printf("sizeof(struct device)     = %zu\n", sizeof(struct device));
    printf("sizeof(struct ext_node)   = %zu\n", sizeof(struct ext_node));
    printf("offsetof(node)            = %zu\n", offsetof(struct device, node));
    printf("offsetof(irq)             = %zu\n", offsetof(struct device, irq));
    printf("\n");

    /* 侵入式: 3 个设备直接放栈上, 零分配 */
    struct dev_list_head { struct dev_list_head *next, *prev; } dev_head;
    /* 用 list_head 更直接 */
    struct list_head dev_head_l;
    INIT_LIST_HEAD(&dev_head_l);

    struct device d0 = { 0, "uart0", {}, 30 };
    struct device d1 = { 1, "spi1",  {}, 31 };
    struct device d2 = { 2, "i2c2",  {}, 32 };
    INIT_LIST_HEAD(&d0.node);
    INIT_LIST_HEAD(&d1.node);
    INIT_LIST_HEAD(&d2.node);

    list_add_tail(&d0.node, &dev_head_l);
    list_add_tail(&d1.node, &dev_head_l);
    list_add_tail(&d2.node, &dev_head_l);

    printf("--- 侵入式遍历 (零 malloc) ---\n");
    struct device *dp;
    list_for_each_entry(dp, &dev_head_l, node) {
        printf("  id=%d name=%s irq=%u  (addr=%p, node_off=%zu)\n",
               dp->id, dp->name, dp->irq, (void*)dp, offsetof(struct device, node));
    }

    /* 外挂式: 每个节点都要 malloc */
    struct ext_node *ext_head = NULL;
    struct ext_node *e0 = malloc(sizeof(*e0));
    struct ext_node *e1 = malloc(sizeof(*e1));
    struct ext_node *e2 = malloc(sizeof(*e2));
    e0->data = &d0; e0->next = ext_head; ext_head = e0;
    e1->data = &d1; e1->next = ext_head; ext_head = e1;
    e2->data = &d2; e2->next = ext_head; ext_head = e2;

    printf("\n--- 外挂式遍历 (3 次 malloc) ---\n");
    struct ext_node *ep;
    for (ep = ext_head; ep; ep = ep->next) {
        printf("  node=%p data=%p id=%d name=%s\n",
               (void*)ep, (void*)ep->data, ep->data->id, ep->data->name);
    }

    printf("\n=== 内存布局对比 ===\n");
    printf("侵入式: d0=%p  node=%p  (node 在宿主内部, 距离=%zd)\n",
           (void*)&d0, (void*)&d0.node,
           (char*)&d0.node - (char*)&d0);
    printf("外挂式: e0=%p  data=%p  (node 与数据分离, 距离=%zd)\n",
           (void*)e0, (void*)e0->data,
           (char*)e0->data - (char*)e0);

    free(e0); free(e1); free(e2);
    return 0;
}
