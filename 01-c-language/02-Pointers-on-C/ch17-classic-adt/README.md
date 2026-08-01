# 第 17 章 经典抽象数据类型

**Classic Abstract Data Types**

## 本章讲什么

全书**综合收官**：不透明 ADT、**栈/队列/Ring/树/哈希**、**void\* + 回调**、内存全生命周期。DPDK **rte_ring**/mempool、内核 list、HFT 订单队列的设计范式。

## 学习重点

- **封装 + 接口隔离**：`.h` 句柄，`.c` 实现  
- **三大回调**：cmp / handler / free_cb  
- **链表队列** vs **环形数组**（rte_ring 原型）  
- **Ring 索引 `% cap`**；SPSC 无锁  
- **BST** + **bsearch** 有序检索；**哈希** O(1) 映射  
- destroy **必传 free_cb**；realloc 临时指针  
- 多线程链表需同步；数据面预分配 Ring  

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | rte_ring、mempool 链表、不透明 mbuf 句柄 |
| 内核 | list_head、slab 池、设备队列 |
| HFT | 订单 Ring、合约哈希、撮合队列 |

## 线上陷阱（汇总）

1. 暴露 struct 成员改 head/tail  
2. destroy 无 free_cb 泄漏  
3. Ring 未取模越界  
4. 回调强转错  
5. 无锁并发改链表  
6. realloc 丢指针  

## 实操（建议完成）

1. 不透明链表 ADT + free_cb  
2. SPSC Ring 入出队  
3. 有序数组 + bsearch  
4. 链地址哈希表  
5. 无 free_cb 泄漏 vs 修复  
6. Ring 去掉取模复现越界  
7. 业务代码切换链表/Ring 底层  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06–ch16 全书综合 |
| 后序 | ch18 运行时/堆模型 |
| 配套 | 《C陷阱与缺陷》ch03、ch05 |

## 小节

- [17.1 内存分配](./17.1-内存分配.md)
- [17.2 堆栈](./17.2-stacks/17.2-stacks.md)
- [17.3 队列](./17.3-queues/17.3-queues.md)
- [17.4 树](./17.4-trees/17.4-trees.md)
- [17.5 实现的改进](./17.5-implementation-improvements/17.5-implementation-improvements.md)
