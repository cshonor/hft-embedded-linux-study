# Ch 6 内核数据结构 · Kernel Data Structures

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：内核内置 **链表 / 队列 / 映射 / 红黑树** — **别重新发明轮子**。读 Ch 3 `task_struct` 链表、Ch 4 CFS `rbtree`、驱动与 IPC 代码时的 **词汇表**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 链表** | `list_head` | **嵌入节点** · `container_of` |
| **② 队列** | `kfifo` | **生产者/消费者** · FIFO |
| **③ 映射** | `idr` | **UID → 指针** |
| **④ 红黑树** | `rbtree` | **O(log n)** 查找 |
| **⑤ 选型指南** | When to use what | 四类场景对照 |
| **⑥ 复杂度** | Big-O | **O(1) / O(n) / O(log n)** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 设计原则 | [notes/section-6.1-设计原则.md](./notes/section-6.1-设计原则.md) |
| 链表 | [notes/section-6.2-链表.md](./notes/section-6.2-链表.md) |
| 队列 | [notes/section-6.3-队列.md](./notes/section-6.3-队列.md) |
| 映射 | [notes/section-6.4-映射.md](./notes/section-6.4-映射.md) |
| 二叉树 | [notes/section-6.5-二叉树.md](./notes/section-6.5-二叉树.md) |
| 选择合适的数据结构 | [notes/section-6.6-选择合适的数据结构.md](./notes/section-6.6-选择合适的数据结构.md) |
| 算法复杂度 | [notes/section-6.7-算法复杂度.md](./notes/section-6.7-算法复杂度.md) |

---

## 本章小结

| 结构 | 核心 API / 宏 | 典型场景 |
|------|---------------|----------|
| **链表** | `list_head`, `list_for_each_entry` | 任务表、等待队列 |
| **队列** | `kfifo_in/out` | 中断→线程、流缓冲 |
| **映射** | `idr` | 整数 ID → 指针 |
| **红黑树** | `rb_*`, 自建比较 | CFS、interval tree |
| **原则** | 用内核现成的 | 别重复造轮子 |

---

## 本章学习目标 · 自检

- [ ] 解释 **嵌入 `list_head`** 与 `container_of` / `list_entry`
- [ ] 说出 **`kfifo`** 适合的生产者/消费者形态
- [ ] 区分 **`idr`**（UID→ptr）与 **`rbtree`**（有序键）
- [ ] 知道 **rbtree 无泛型插入** — 为何要手写比较函数
- [ ] 用 Big-O 论证：为何 CFS 用树而不是链表扫最小 `vruntime`

---

## 相关章节

- 上一章：[../chapter-05-system-calls/](../chapter-05-system-calls/)
- 下一章：[../chapter-07-interrupts/](../chapter-07-interrupts/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
