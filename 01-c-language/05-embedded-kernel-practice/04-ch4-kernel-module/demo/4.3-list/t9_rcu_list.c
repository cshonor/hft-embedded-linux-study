/* T9: RCU 链表模拟 —— list_add_rcu vs list_add 的写入次数对比
 *
 * RCU (Read-Copy-Update): 读端无锁, 写端先复制再原子替换.
 * 链表层面:
 *   list_add_rcu 的关键是写入顺序: 先建好 new->next, wmb, 再改 prev->next
 *   读者要么看到旧链, 要么看到新链, 不会看到半成品
 *
 * 这里只统计关键路径写入次数, 不做实际遍历 (避免 container_of 野指针问题)
 */
#include <stdio.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };
static inline void INIT_LIST_HEAD(struct list_head *list) { list->next = list; list->prev = list; }

/* 普通版: 4 个指针写入 (无顺序保证) */
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* RCU 版: 同样写 4 个指针, 但 next 方向的写入有 wmb 屏障 */
static inline void __list_add_rcu(struct list_head *new,
                                  struct list_head *prev,
                                  struct list_head *next) {
    new->next = next;       /* 1: 先建好 new 的 next */
    new->prev = prev;       /* 2: 设置 prev */
    /* smp_wmb(); */          /* 3: 写屏障: 上面的必须在下面的之前可见 */
    next->prev = new;       /* 4: 修复后继 */
    prev->next = new;       /* 5: 最后让链表"看到" new (原子点) */
}

int main(void)
{
    printf("=== T9: RCU 链表 vs 普通链表 (写入对比) ===\n\n");

    /* 模拟统计: 普通版 4 次写入, RCU 版 4 次写入 + 1 次 wmb */
    struct list_head head, a, b;
    INIT_LIST_HEAD(&head);
    INIT_LIST_HEAD(&a);
    INIT_LIST_HEAD(&b);

    /* 普通版 */
    __list_add(&a, &head, head.next);
    __list_add(&b, &head, head.next);
    printf("普通 list_add: 每次插入写 4 个指针 (无屏障)\n");
    printf("  next->prev = new\n  new->next  = next\n  new->prev  = prev\n  prev->next = new\n\n");

    /* RCU 版 */
    INIT_LIST_HEAD(&head);
    INIT_LIST_HEAD(&a);
    INIT_LIST_HEAD(&b);
    __list_add_rcu(&a, &head, head.next);
    __list_add_rcu(&b, &head, head.next);
    printf("RCU list_add_rcu: 每次插入写 4 个指针 + 1 次 smp_wmb\n");
    printf("  new->next  = next   (先建好 next)\n");
    printf("  new->prev  = prev   (设 prev)\n");
    printf("  [smp_wmb]           (写屏障!)\n");
    printf("  next->prev = new   (修复后继)\n");
    printf("  prev->next = new   (最后才让链表看到 new)\n\n");

    printf("=== RCU 的核心语义 ===\n");
    printf("  写端: 先建好 new->next, wmb, 再原子改 prev->next\n");
    printf("        读者要么看到旧链, 要么看到新链, 不会看到\"半插入\"的节点\n");
    printf("  读端: 只顺 next 走 (不读 prev), 不需要锁\n");
    printf("        list_for_each_entry_rcu 用 rcu_dereference 读 next (防重排)\n");
    printf("  代价: 删除时要等所有读者结束 (grace period) 才能真正释放\n");
    printf("        -> list_del_rcu 只摘链, synchronize_rcu() 后才 kfree\n\n");

    printf("=== 适用场景 ===\n");
    printf("  读多写少 (如路由表、inode 缓存): 读端零开销, 写端稍慢\n");
    printf("  读写都频繁: 用普通 spinlock 保护的链表更简单\n");
    return 0;
}
