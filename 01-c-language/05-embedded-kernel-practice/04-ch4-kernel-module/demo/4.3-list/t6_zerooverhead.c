/* T6: 零开销验证 —— container_of + list_for_each_entry 在 -O2 下的代码生成
 *
 * 验证: container_of 在 -O2 下是单条 lea, list_for_each_entry 展开后
 *       没有额外函数调用, 没有额外内存访问.
 */
#include <stdio.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct device {
    char             pad[64];
    int              irq;
    struct list_head node;      /* offset 72 */
    unsigned long long stamp;
};

/* 这个函数在 -O2 下应该只剩 lea -0x48(%rdi),%rax; ret */
struct device *get_host(struct list_head *node) {
    return container_of(node, struct device, node);
}

/* list_next_entry 展开: container_of(pos->node.next, ...) */
struct device *get_next(struct device *pos) {
    return list_entry(pos->node.next, struct device, node);
}

/* 链表遍历的累加: 验证没有额外开销 */
int sum_irqs(struct list_head *head) {
    struct device *pos;
    int sum = 0;
    for (pos = list_entry(head->next, __typeof__(*pos), node);
         &pos->node != head;
         pos = list_entry(pos->node.next, __typeof__(*pos), node)) {
        sum += pos->irq;
    }
    return sum;
}

int main(void)
{
    printf("=== T6: 零开销验证 (看反汇编) ===\n");
    printf("offsetof(node) = %zu (0x48 = 72)\n", offsetof(struct device, node));
    printf("sizeof(struct device) = %zu\n", sizeof(struct device));
    printf("\n  反汇编: objdump -d t6_zerooverhead.o | sed -n '/<get_host>:/,/^$/p'\n");
    printf("  预期: lea -0x48(%%rdi),%%rax + ret  (一条 lea)\n");

    /* 跑一下确保逻辑对 */
    struct device d = { {0}, 42, {}, 0 };
    d.node.next = &d.node; d.node.prev = &d.node;
    struct device *back = get_host(&d.node);
    printf("  get_host(&d.node) == &d ? %s\n", back == &d ? "yes" : "NO");
    printf("  get_next(&d) == &d ? %s\n", get_next(&d) == &d ? "yes" : "NO");
    printf("  sum_irqs(&d.node) = %d (expect 42)\n", sum_irqs(&d.node));
    return 0;
}
