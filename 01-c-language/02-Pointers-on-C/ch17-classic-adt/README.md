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


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 环形数组队列

```c
#define CAP 8
int buf[CAP];
int head = 0, tail = 0;

// 入队
void enqueue(int val) {
    tail = (tail + 1) % CAP;
    buf[tail] = val;
}

// 队列容量是多少？满了的判断条件？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 容量是 `CAP - 1 = 7`（牺牲一个位置区分空和满）。满：`(tail + 1) % CAP == head`。空：`head == tail`。

**DPDK `rte_ring`** 原型就是这个模式，用无锁 CAS 实现多生产者/多消费者。

**复习：** → [17.3 Queues](./17.3-queues/17.3-queues.md)

</details>

### Q2: 不透明类型封装

```c
// queue.h
typedef struct Queue Queue;  // 前向声明
Queue *queue_create(void);
void   queue_destroy(Queue *q);
int    queue_push(Queue *q, int val);

// queue.c
struct Queue {               // 完整定义只在此
    int data[100];
    int head, tail;
};
```

> 为什么 `.h` 只暴露 `Queue` 而不暴露 `struct Queue` 的定义？

<details>
<summary>答案与复习指引</summary>

**答案：** **不透明指针**（opaque pointer）模式——调用者只知道 `Queue*` 是个指针，不能直接访问成员。好处：
1. **封装** — 调用者不能绕过接口直接修改 `head`/`tail`
2. **ABI 稳定** — 改 `struct Queue` 内部布局不需重新编译使用者
3. **隐藏实现** — `.h` 不暴露内部数据结构

**DPDK/内核大量使用：** `struct rte_mempool`、`struct file` 等。

**复习：** → [17.1 Stacks](./17.1-stacks.md) — ADT 封装

</details>
