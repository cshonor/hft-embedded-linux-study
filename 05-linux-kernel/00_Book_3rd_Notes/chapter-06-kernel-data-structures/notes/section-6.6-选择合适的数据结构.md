## ⑤ 选择合适的数据结构

| 主要需求 | 选用 |
|----------|------|
| **遍历所有元素** | **链表** `list_head` |
| **生产者 / 消费者、FIFO** | **队列** `kfifo` |
| **UID → 对象指针** | **映射** `idr` |
| **大量数据 + 高效随机检索** | **红黑树** `rbtree` |

#### 决策简图

```
要存内核对象？
    │
    ├─ 主要靠「扫一遍」──────► list_head
    ├─ 一边产一边消、FIFO ───► kfifo
    ├─ 要小整数 handle ──────► idr
    └─ 按键排序/最值/查找 ───► rbtree
```

| 组合 | 现实例子 |
|------|----------|
| list + rbtree | 同一任务既在 **全局链表** 又在 **CFS 树** |
| kfifo + workqueue | 中断 **in**，线程 **out** 处理 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核中遍历所有进程用什么数据结构？查找特定 PID 用什么？

<details><summary>答案</summary>

遍历所有进程：task_struct 通过 tasks 链表串联，list_for_each 遍历。查找特定 PID：用 PID 哈希表（`find_task_by_vpid()`），O(1) 查找。如果只有链表，查找 PID 需要 O(n) 遍历所有进程，系统中千个进程时太慢。

</details>

**Q2.** HFT 限价单簿应该用什么数据结构？

<details><summary>答案</summary>

限价单簿需要：按价格排序（找最优价 O(1)）、按价格查找（O(log n)）、插入/取消订单（O(log n)）。红黑树满足：找最优价 = 最左/最右节点，插入/删除 O(log n)。补充：同一价位多笔订单用链表挂在红黑树节点上。高频场景可用数组+堆优化（完全二叉树 cache 友好）。

</details>

</details>
---
