## 设计原则

内核提供通用数据结构原语 → **鼓励重用**。

| 建议 | 原因 |
|------|------|
| **用内核现成结构** | 经审计、一致、少 bug |
| **勿 roll your own** | 链表/树写错一处 → 难查的内存破坏 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核为什么要提供自己的链表/队列/映射，而不直接用 C 标准库？

<details><summary>答案</summary>

1) 内核没有 libc，不能用标准库；2) 内核数据结构需要考虑 SMP 安全（自旋锁/RCU）、内存效率（嵌入式设计无数据载荷指针）、实时性（O(1) 操作）。标准库的数据结构不考虑这些内核特有约束。

</details>

**Q2.** 内核数据结构的「嵌入式设计」是什么意思？

<details><summary>答案</summary>

标准链表节点包含数据指针；内核 list_head 嵌入在数据结构内部（如 struct task_struct { struct list_head tasks; }）。通过 container_of 宏从 list_head 反推宿主结构地址。优点：零额外内存分配、一个对象可挂多个链表、类型安全。

</details>

</details>
---
