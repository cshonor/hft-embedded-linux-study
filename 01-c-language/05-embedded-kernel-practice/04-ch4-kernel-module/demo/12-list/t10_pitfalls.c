/* T10: 边界与坑点 —— 类型安全弱 + 空链表安全 + use-after-del
 *
 * 1. 成员名写错但类型相同 -> 零警告, 地址算错 (已在 CH1 6.4 详述, 这里验证)
 * 2. 空链表遍历 -> 安全 (head.next == head, 循环不执行)
 * 3. list_del 后 next 指向 POISON -> 后续解引用 segfault (快速暴露 bug)
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#pragma GCC diagnostic ignored "-Warray-bounds"

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

struct device {
    int id;
    int irq;          /* 与 id 同类型 (int) */
    char name[8];
    struct list_head node;
};

int main(void)
{
    printf("=== T10: 边界与坑点 ===\n\n");

    struct device d = { .id = 7, .irq = 30 };
    strcpy(d.name, "uart");
    INIT_LIST_HEAD(&d.node);

    /* 坑 1: 成员名写错 (id vs irq), 但类型相同 -> 零警告
     * (详见 CH1 6.4.3 类型检查边界; gcc -O2 -Warray-bounds 会拦, 这里不演示) */
    printf("--- 坑 1: 成员名写错 (id 传给 irq) ---\n");
    printf("  &d.id=%p  offsetof(irq)=%zu  &d=%p\n",
           (void*)&d.id, offsetof(struct device, irq), (void*)&d);
    printf("  container_of(&d.id, device, irq) 会算出 &d - 4 = %p (地址错!)\n",
           (void*)((char*)&d.id - offsetof(struct device, irq)));
    printf("  -> 零警告! 编译器只检查类型 (int==int), 不检查成员名\n");
    printf("  -> gcc -O2 -Warray-bounds 会检测到越界, 但默认只 warning\n");

    /* 坑 2: 空链表遍历安全 */
    printf("\n--- 坑 2: 空链表遍历安全 ---\n");
    struct list_head empty;
    INIT_LIST_HEAD(&empty);
    printf("  empty.next=%p empty.prev=%p (自指)\n", (void*)empty.next, (void*)empty.prev);
    struct device *pos;
    int count = 0;
    list_for_each_entry(pos, &empty, node) { count++; (void)pos; }
    printf("  遍历空链表 count=%d (安全, 循环体不执行)\n", count);

    /* 坑 3: list_del 后 next=POISON */
    printf("\n--- 坑 3: list_del 后 next=POISON ---\n");
    struct list_head head;
    INIT_LIST_HEAD(&head);
    list_add_tail(&d.node, &head);
    /* 模拟 list_del (投毒) */
    d.node.next = (void *)0x100;
    d.node.prev = (void *)0x122;
    printf("  d.node.next=%p d.node.prev=%p (已投毒)\n",
           (void*)d.node.next, (void*)d.node.prev);
    printf("  如果再遍历到 d: pos = container_of(0x100, device, node)\n");
    printf("    -> 解引用 0x100 - offsetof = %p -> segfault\n",
           (void *)((char *)0x100 - offsetof(struct device, node)));
    printf("  -> list_del 的设计意图: 快速暴露 use-after-del\n\n");

    printf("=== 坑点总结 ===\n");
    printf("| # | 坑              | 编译器反应 | 后果          |\n");
    printf("|---|-----------------|------------|---------------|\n");
    printf("| 1 | 成员名写错(同类型)| 零警告     | 地址算错, 读错 |\n");
    printf("| 2 | 宿主类型写错     | 零警告     | 读到别的内存   |\n");
    printf("| 3 | 传 NULL           | 零警告     | 野指针 (UBSan抓) |\n");
    printf("| 4 | 空链表遍历       | 安全       | 循环不执行     |\n");
    printf("| 5 | use-after-del    | POISON     | 快速 segfault  |\n");
    printf("| 6 | const 丢失        | 需-Wcast-qual | 写穿 const   |\n");
    return 0;
}
