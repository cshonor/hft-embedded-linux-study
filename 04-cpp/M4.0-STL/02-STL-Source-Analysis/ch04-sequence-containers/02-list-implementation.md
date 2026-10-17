# 4.2 list 源码：环形链表与哨兵

> 第 4 章 序列容器 · 第 2 节 · 上一节：[4.1 vector 源码](01-vector-implementation.md) · 下一节：[4.3 deque 源码](03-deque-implementation.md)

## 为什么要学这个（先建立直觉）

在 C 里，双向链表要手写节点结构和插入/删除函数，还要处理空链表的特殊情况。SGI STL 的 list 用环形链表 + 哨兵节点统一了所有边界条件。

```c
/* C: 手写双向链表 */
struct Node { int val; struct Node *prev, *next; };
struct Node* head = NULL;  // 空链表特判

void insert(struct Node* pos, int val) {
    struct Node* n = malloc(sizeof(struct Node));
    n->val = val;
    if (head == NULL) {  // 空链表特殊处理
        head = n; n->prev = n->next = n;
    } else {
        n->prev = pos->prev;
        n->next = pos;
        pos->prev->next = n;
        pos->prev = n;
    }
}
```

```cpp
// SGI list: 环形 + 哨兵，无需空链表特判
// 哨兵节点始终存在，空链表 = 哨兵自指
// end() 返回哨兵，begin() 返回哨兵的 next
```

**直觉**：SGI list 总有一个哨兵节点（`end()` 指向它），空链表时哨兵自指。插入/删除不需要特判空链表——所有操作统一处理。

## 这节讲什么

### 节点结构

```cpp
template<typename T>
struct __list_node {
    void* prev;  // 前驱（void* 是 SGI 的做法，实际是 __list_node*）
    void* next;  // 后继
    T data;      // 数据
};
```

### 环形 + 哨兵

```cpp
template<typename T, typename Alloc = std::allocator<T>>
class list {
    __list_node<T>* node;  // 哨兵节点（end() 指向它）
public:
    list() {
        node = allocate_node();  // 分配哨兵
        node->next = node;       // 空链表：哨兵自指
        node->prev = node;
    }
    iterator begin() { return (iterator)(node->next); }
    iterator end() { return (iterator)node; }
    bool empty() const { return node->next == node; }
};
```

```
空链表：
  [sentinel] ←→ [sentinel]  (自指)

3 个元素的链表：
  [sentinel] ↔ [A] ↔ [B] ↔ [C] ↔ [sentinel]
       ↑                                  ↑
   end()                           end()（同一个节点）
   begin() = sentinel->next = [A]
```

### 插入操作（无特判）

```cpp
void insert(iterator pos, const T& val) {
    __list_node<T>* n = allocate_node();
    construct(&n->data, val);
    n->next = pos.node;
    n->prev = pos.node->prev;
    (__list_node<T>*)pos.node->prev->next = n;
    pos.node->prev = n;
    // 不需要 if (empty()) 特判！
    // 空链表时 pos = end() = sentinel，上面代码正确工作
}
```

### 迭代器不失效

```cpp
// list 插入/删除不影响其他迭代器（节点不搬迁）
std::list<int> l = {1, 2, 3};
auto it = l.begin();  // 指向 1
l.push_back(4);       // it 仍有效，仍指向 1
l.insert(l.begin(), 0);  // it 仍有效
// 对比 vector：push_back 可能扩容 → it 失效
```

### 成员 sort（归并排序）

```cpp
std::list<int> l = {3, 1, 4, 1, 5};
l.sort();  // 成员函数，归并排序 O(n log n)
// std::sort(l.begin(), l.end());  // 编译错误：需要 RandomAccessIterator
```

## 常见错误（新手踩坑）

### 错误 1：用 std::sort 排 list

```cpp
std::list<int> l = {3, 1, 4};
std::sort(l.begin(), l.end());  // 编译错误
```

### 错误 2：以为 list 随机访问 O(1)

```cpp
auto it = l.begin();
std::advance(it, 1000);  // O(1000)，不是 O(1)
// list 迭代器是 Bidirectional，只能 ++/--，不能 +n
```

### 错误 3：在循环中用 erase

```cpp
for (auto it = l.begin(); it != l.end(); ) {
    if (*it < 0) it = l.erase(it);  // erase 返回下一个迭代器
    else ++it;
}
// 如果写成 l.erase(it); ++it; → it 已失效，UB
```

## 新手要点（和 C 的区别）

| 方面 | C (手写链表) | C++ list |
|------|-------------|----------|
| 空链表 | 特判 NULL | 哨兵自指，无特判 |
| 内存管理 | 手动 free | RAII 自动释放 |
| 迭代器失效 | 指针失效 | 插删不影响其他迭代器 |
| 排序 | 手写归并 | 成员 sort() |

## HFT 关联

- **list 迭代器不失效 = 稳定句柄**：订单簿挂单/撤单需要稳定引用，list 满足。但 cache 代价高。
- **侵入式链表替代**：HFT 常用 Linux `list_head`（侵入式链表）+ 连续 vector 池，兼顾稳定句柄和 cache 友好
- **list 的 cache 代价**：每个节点非连续 → 每次跳转一次 cache miss，热路径禁用纯 list

## 代码自测

### Q1: 哨兵节点

```cpp
std::list<int> l;
std::list<int>::iterator end = l.end();
l.push_back(1);
l.push_back(2);
// end 还有效吗？指向什么？
```

<details>
<summary>答案</summary>

**有效**。`end` 仍指向哨兵节点。list 的插入/删除不影响其他迭代器（节点不搬迁），哨兵始终存在。

```
push_back 前：[sentinel]（自指），end → sentinel
push_back 后：[sentinel] ↔ [1] ↔ [2] ↔ [sentinel]，end → 仍指向 sentinel
```

**对比 vector**：vector 的 `end()` 在 push_back 后会变（finish 移动），但 list 的 `end()` 永远指向哨兵。
</details>

### Q2: 迭代器不失效

```cpp
std::list<int> l = {1, 2, 3, 4, 5};
auto it2 = std::next(l.begin(), 1);  // 指向 2
auto it4 = std::next(l.begin(), 3);  // 指向 4

l.erase(it2);  // 删除 2
// it4 还有效吗？
std::cout << *it4;  // 输出什么？
```

<details>
<summary>答案</summary>

**有效**，输出 **4**。

list 删除节点不影响其他节点的迭代器——`it4` 指向的节点 4 没有被移动或释放，只是节点 2 被摘除了。

```
删除前：[1] ↔ [2] ↔ [3] ↔ [4] ↔ [5] ↔ [sentinel]
删除后：[1] ↔ [3] ↔ [4] ↔ [5] ↔ [sentinel]
                      ↑
                    it4 仍指向 4
```

**对比 vector**：vector erase 后，被删元素之后的迭代器全部失效（元素前移）。list 不搬迁节点，所以其他迭代器不受影响。
</details>

### Q3: list sort

```cpp
std::list<int> l = {5, 3, 1, 4, 2};
l.sort();  // 成员函数
// 用了什么算法？复杂度？
```

<detailf>
<summary>答案</summary>

用**归并排序**（merge sort），复杂度 **O(n log n)**。

为什么不用 std::sort？因为 std::sort 需要 RandomAccessIterator（支持 `+n`/`[]`），list 只有 BidirectionalIterator。归并排序只需要 `++`/`--` 和 splice（O(1) 节点搬移），适合链表。

```cpp
// list::sort 的简化逻辑
void sort() {
    // 分治：把链表对半分 → 递归排序 → 归并
    // 利用 splice O(1) 搬节点，无需拷贝数据
}
```

**对比 vector sort**：vector 用内省排序（快排+堆排+插入排序），因为随机访问支持 pivot 选择和 partition。
</details>

### Q4: list vs vector 选型

```
HFT 订单簿需要：
1. 快速插入/删除挂单（O(1)）
2. 稳定的订单引用（迭代器不失效）
3. 按价格排序
4. 高频遍历档位
```
> list 和 vector 哪个更适合？

<details>
<summary>答案</summary>

**两者都不完美**：

| 需求 | list | vector |
|------|------|--------|
| O(1) 插删 | ✅（已知位置） | ❌（需搬移） |
| 稳定引用 | ✅（迭代器不失效） | ❌（扩容失效） |
| 按价格排序 | ✅ 成员 sort | ✅ std::sort |
| 高频遍历 | ❌（cache miss 严重） | ✅（连续内存） |

**HFT 实际方案**：侵入式链表 + 连续 vector 池：
1. 订单对象存在 `vector<Order>` 中（连续存储，cache 友好）
2. 用侵入式链表（如 `list_head`）按价格串联订单的索引
3. 挂单/撤单操作链表（O(1)），遍历走 vector（cache 命中）

```cpp
struct Order {
    int price, quantity;
    struct list_head node;  // 侵入式链表节点
};
std::vector<Order> pool;  // 连续存储
// list_head 串联 pool 中的订单，索引不失效（vector 不扩容因为 reserve）
```
</details>

## 参考与延伸

- 上一节：[4.1 vector 源码](01-vector-implementation.md)
- 下一节：[4.3 deque 源码](03-deque-implementation.md)
