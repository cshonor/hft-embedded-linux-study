/* T8: 宏展开实测 —— list_for_each_entry 宏完全展开后的代码
 *
 * 验证: list_for_each_entry(pos, head, member) 宏展开后:
 *   for (pos = list_entry(head->next, typeof(*pos), member);   // 初始化
 *        &pos->member != head;                                   // 条件
 *        pos = list_entry(pos->member.next, typeof(*pos), member)) // 步进
 *
 * 进一步展开 list_entry -> container_of -> typeof + offsetof
 *
 * 用 -E 预处理看完整展开, 用 objdump 看生成的指令.
 */
#include <stdio.h>
#include <stddef.h>

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

/* 这个函数被 list_for_each_entry 展开 */
int sum_ids(struct list_head *head)
{
    struct item *pos;
    int sum = 0;
    list_for_each_entry(pos, head, node) {
        sum += pos->id;
    }
    return sum;
}

int main(void)
{
    printf("=== T8: 宏展开实测 ===\n\n");
    printf("list_for_each_entry(pos, head, node) 展开为:\n\n");
    printf("  for (pos = container_of(head->next, typeof(*pos), node);\n");
    printf("       &pos->node != head;\n");
    printf("       pos = container_of(pos->node.next, typeof(*pos), node))\n\n");
    printf("  每次步进 = 一次 container_of = 一次 typeof(编译期) + 一次 lea(运行期)\n");
    printf("  -> typeof 不生成代码, lea 是 1 周期指令\n");

    /* 跑一下确保对 */
    struct list_head head;
    INIT_LIST_HEAD(&head);
    struct item a = { 1, {} }, b = { 2, {} }, c = { 3, {} };
    INIT_LIST_HEAD(&a.node); INIT_LIST_HEAD(&b.node); INIT_LIST_HEAD(&c.node);
    list_add_tail(&a.node, &head);
    list_add_tail(&b.node, &head);
    list_add_tail(&c.node, &head);
    printf("\n  sum_ids = %d (expect 6)\n", sum_ids(&head));

    printf("\n  预处理: gcc -E t8_macro_expand.c | tail\n");
    printf("  反汇编: gcc -O2 -c t8_macro_expand.c; objdump -d t8.o\n");
    return 0;
}
