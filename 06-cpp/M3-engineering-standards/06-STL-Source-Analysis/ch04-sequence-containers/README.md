# 第 4 章 序列容器

**Sequence Containers**

## 本章讲什么

`vector`/`list`/`deque` 是 STL 三大序列容器，内存模型截然不同：`vector` 连续、`list` 节点链、`deque` 分段连续。本章剖析源码级的内存布局与扩容机制，解释三者性能特性的根源。

## 要点

### `vector`：连续 + 三指针

```cpp
T* start;        // 已用起点
T* finish;       // 已用终点(size)
T* end_of_storage; // 容量终点(capacity)
```
- 随机访问 O(1)（指针算术）；尾插均摊 O(1)，扩容翻倍。
- 迭代器是原生指针 `T*`（随机访问）。
- 扩容 = 新分配 + 元素移动/拷贝 + 旧释放，迭代器全部失效。

### `list`：双向链表 + 环形 + 哨兵

- 节点 `struct node { node* prev; node* next; T data; };`
- 环形链表 + 一个哨兵节点（`end()` 指向它），空链表哨兵自指。
- 插删 O(1)（已知位置）；随机访问 O(n)（只能遍历）。
- 迭代器是双向（`--`/`++`），非随机访问——不能用 `std::sort`，用成员 `sort()`（归并）。
- **不失效**：插删不影响其他迭代器（节点不搬迁）。

### `deque`：分段连续 + 中控器

- 一段连续的 `map`（指针数组），每段指向一块连续缓冲区。
- 随机访问：`map[node] + offset`，两步间接——逻辑连续但物理分段。
- 头尾双端扩容（`push_front`/`push_back` 各自扩缓冲区/map），中部插删 O(n)。
- 迭代器复杂（跨段时跳到下一段缓冲区）。

### `stack`/`queue`：适配器

非独立容器，是 `deque`（默认）或 `list` 的接口裁剪：`stack` 只露 `top`/`push`/`pop`；`queue` 只露 `front`/`back`/`push`/`pop`。无迭代器。

## HFT 关联

- **`vector` 连续换 cache**：tick 缓冲、订单档位用 `vector`，顺序遍历 cache 命中率高；`list` 的指针追逐每节点一次 cache miss，热路径禁用。
- **`deque` 的两步间接**：随机访问要两次访存（map→buffer），比 `vector` 一次访存慢——HFT 需要严格 O(1) 单步访问时选 `vector`。
- **`list` 迭代器不失效**：订单簿挂单/撤单需要稳定句柄（迭代器不失效），`list` 满足；但 cache 代价让 HFT 更倾向侵入式链表（`list_head`，见《C 和指针》ch17）+ 连续 `vector` 池。
- **扩容翻倍的延迟尖峰**：`vector` 扩容有 O(n) 搬迁尖峰，HFT 启动 `reserve` 消除。

## 自测题

1. `vector` 的三个指针分别代表什么？扩容时迭代器为什么全部失效？
2. `list` 为什么是环形 + 哨兵？这种设计让空链表判断和插删有什么便利？
3. `deque` 如何实现"逻辑连续物理分段"的随机访问？为什么比 `vector` 慢？
4. `stack`/`queue` 是独立容器吗？默认底层是什么？
5. HFT 为什么热路径用 `vector` 而非 `list`？`list` 的什么特性仍让它在订单簿挂单中有价值？
