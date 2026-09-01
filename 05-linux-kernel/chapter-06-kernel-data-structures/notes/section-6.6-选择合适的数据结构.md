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
    ├─ 主要靠「扫一遍」─────────► list_head
    ├─ 一边产一边消、FIFO ──────► kfifo（SPSC 无锁）
    ├─ 要小整数 handle ─────────► idr → 现代 xarray
    ├─ 按键排序/最值/查找 ──────► rbtree
    ├─ 按键范围查询（区间）─────► maple tree（v6.1+）/ interval tree
    └─ 均匀键、O(1) 点查 ───────► hlist 哈希表
```

| 组合 | 现实例子 |
|------|----------|
| list + rbtree | 同一任务既在 **全局链表** 又在 **CFS 树** |
| kfifo + workqueue | 中断 **in**，线程 **out** 处理 |
| xarray + tag | page cache：页索引树 + dirty/writeback 标记位 |
| maple tree + per-VMA lock | VMA 管理（v6.1+/v6.4+，见 [15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)） |

#### 选型时别忘了第四个维度：并发形状

复杂度只是入场券，**结构怎么被共享**往往才是决定因素：

| 并发形状 | 首选 |
|----------|------|
| 单 CPU 独占数据 | 任何结构——**per-CPU 化**直接消灭共享（`alloc_percpu`，见 [12.10](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)） |
| SPSC 跨上下文（中断→线程） | kfifo 无锁环 |
| 读极多写极少 | RCU 保护的任何结构（读侧零开销，见 [Ch 9](../../chapter-09-kernel-sync-intro/)） |
| 多写者有序键 | rbtree/xarray + spinlock；或 maple tree |

#### 反面教材：选错结构的代价史

| 案例 | 教训 |
|------|------|
| 早期 O(n) 调度器 | 进程一多调度延迟不可预测 → O(1) 调度器（位图）→ CFS（rbtree）→ maple 时代仍在演化（见 [4.1](../../chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md)） |
| 2.6 VMA 链表+红黑树双簿记 | 每次增删两份维护 → v6.1 maple tree 一份搞定（见 [15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)） |
| radix tree + 外挂锁 | page cache 改 xarray：锁与 tag 进结构（见 [6.4](./section-6.4-映射.md)） |

**HFT：** 选型逻辑直接平移到用户态——行情网关收包→策略用 SPSC 环（对应 kfifo）；订单簿按价格有序用红黑树/跳表（对应 rbtree + [6.7](./section-6.7-算法复杂度.md) 的缓存维度）；fd 式会话句柄用稀疏数组/句柄表（对应 idr）。**先想并发形状，再想复杂度，最后想缓存布局。**



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
