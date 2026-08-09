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


### Q3: 链表栈实现

```c
struct StackNode {
    int val;
    struct StackNode *next;
};

struct Stack { struct StackNode *top; };

void push(struct Stack *s, int val) {
    struct StackNode *n = malloc(sizeof(*n));
    n->val = val;
    n->next = s->top;    // A: 新节点指向前顶
    s->top = n;           // B: 更新栈顶
}

int pop(struct Stack *s) {
    if (!s->top) return -1;  // 空栈
    struct StackNode *n = s->top;
    int val = n->val;
    s->top = n->next;     // C: 栈顶下移
    free(n);               // D: 释放节点
    return val;
}
```

> 如果删掉 D 行（`free(n)`），会发生什么？push 时 `malloc` 失败怎么办？

<details>
<summary>答案与复习指引</summary>

**答案：** 删掉 D 行 → **内存泄漏**——每次 pop 丢失一个节点，但内存不释放。长时间运行的 HFT 系统会耗尽内存。

`malloc` 失败返回 NULL → `n->val = val` 解引用 NULL → **崩溃**。必须判空：

```c
struct StackNode *n = malloc(sizeof(*n));
if (!n) { /* 处理错误 */ return; }
```

**规则：** 每次 `malloc` 都要判空；每次 `pop`/`delete` 都要 `free`。RAII 思想在 C 中需要手动实现。

**复习：** → [17.1 Stacks](./17.1-stacks.md)

</details>

### Q4: 二叉搜索树查找

```c
struct TreeNode {
    int val;
    struct TreeNode *left, *right;
};

struct TreeNode *find(struct TreeNode *root, int target) {
    while (root) {
        if (target == root->val)
            return root;
        else if (target < root->val)
            root = root->left;     // 往左找
        else
            root = root->right;    // 往右找
    }
    return NULL;  // 没找到
}
```

> BST 查找的时间复杂度是多少？什么情况下退化为 O(n)？如何避免？

<details>
<summary>答案与复习指引</summary>

**答案：**
- 平衡 BST：**O(log n)**——每次排除一半
- 退化为 O(n)：**链表形状**（按有序序列插入，如 1,2,3,4,5 → 每个节点只有右子）

**避免方法：**
- **AVL 树**/**红黑树**：插入/删除时自动旋转保持平衡
- **跳表**：概率平衡的替代方案
- Linux 内核用**红黑树**（`struct rb_node`）管理进程调度、内存管理

**HFT 关联：** 订单簿（Order Book）常用红黑树或跳表——O(log n) 插入/删除/查找。

**复习：** → [17.4 树](./17.4-trees/17.4-trees.md)

</details>

