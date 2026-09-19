/* T2: 一个结构体挂多张链表 —— container_of 的 member 参数决定回推偏移
 *
 * 同一个 struct device 同时挂在 "设备链表" 和 "IRQ 链表" 上,
 * 靠不同的 list_head 成员 (node / irq_node) 区分.
 *
 * 这是侵入式链表最核心的能力: 一份代码, 两种遍历顺序, 零额外内存.
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };

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

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

struct device {
    int              id;
    char             name[12];
    struct list_head node;       /* 挂在设备链表 (偏移 16) */
    unsigned         irq;
    struct list_head irq_node;  /* 同时挂在 IRQ 链表 (偏移 40) */
    int              priority;
};

int main(void)
{
    printf("=== T2: 多链表 (同一结构体挂两张链表) ===\n\n");

    printf("sizeof(struct device)  = %zu\n", sizeof(struct device));
    printf("offsetof(node)         = %zu\n", offsetof(struct device, node));
    printf("offsetof(irq)          = %zu\n", offsetof(struct device, irq));
    printf("offsetof(irq_node)     = %zu\n", offsetof(struct device, irq_node));
    printf("offsetof(priority)     = %zu\n", offsetof(struct device, priority));
    printf("\n");

    struct list_head dev_list, irq_list;
    INIT_LIST_HEAD(&dev_list);
    INIT_LIST_HEAD(&irq_list);

    struct device d0 = { 0, "uart0", {}, 30, {}, 1 };
    struct device d1 = { 1, "spi1",  {}, 31, {}, 2 };
    struct device d2 = { 2, "i2c2",  {}, 32, {}, 0 };
    INIT_LIST_HEAD(&d0.node);  INIT_LIST_HEAD(&d0.irq_node);
    INIT_LIST_HEAD(&d1.node);  INIT_LIST_HEAD(&d1.irq_node);
    INIT_LIST_HEAD(&d2.node);  INIT_LIST_HEAD(&d2.irq_node);

    /* 按 id 顺序挂设备链表 */
    list_add_tail(&d0.node, &dev_list);
    list_add_tail(&d1.node, &dev_list);
    list_add_tail(&d2.node, &dev_list);

    /* 按 priority 倒序挂 IRQ 链表 (priority 2 先, 0 后) */
    list_add_tail(&d1.irq_node, &irq_list);  /* p=2 */
    list_add_tail(&d0.irq_node, &irq_list);  /* p=1 */
    list_add_tail(&d2.irq_node, &irq_list);  /* p=0 */

    printf("--- 设备链表 (按 id 顺序) ---\n");
    struct device *dp;
    list_for_each_entry(dp, &dev_list, node) {
        printf("  id=%d name=%s irq=%u pri=%d (node=%p, irq_node=%p)\n",
               dp->id, dp->name, dp->irq, dp->priority,
               (void*)&dp->node, (void*)&dp->irq_node);
    }

    printf("\n--- IRQ 链表 (按 priority 倒序) ---\n");
    struct device *ip;
    list_for_each_entry(ip, &irq_list, irq_node) {
        printf("  id=%d name=%s irq=%u pri=%d (node=%p, irq_node=%p)\n",
               ip->id, ip->name, ip->irq, ip->priority,
               (void*)&ip->node, (void*)&ip->irq_node);
    }

    printf("\n=== 关键: 同一份 list_for_each_entry 宏, member 参数不同, 回推偏移不同 ===\n");
    printf("  via node      -> offsetof(node)      = %zu\n", offsetof(struct device, node));
    printf("  via irq_node   -> offsetof(irq_node)  = %zu\n", offsetof(struct device, irq_node));

    /* 验证: 从 irq_node 回推, 得到的还是同一个 device */
    struct device *via_irq = list_entry(&d1.irq_node, struct device, irq_node);
    struct device *via_node = list_entry(&d1.node, struct device, node);
    printf("\n  d1 via irq_node = %p  (== &d1 ? %s)\n", (void*)via_irq, via_irq == &d1 ? "yes" : "NO!");
    printf("  d1 via node     = %p  (== &d1 ? %s)\n", (void*)via_node, via_node == &d1 ? "yes" : "NO!");
    return 0;
}
