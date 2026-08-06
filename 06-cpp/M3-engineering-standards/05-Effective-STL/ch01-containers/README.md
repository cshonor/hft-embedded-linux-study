# 第 1 章 容器

**Containers** — Items 1–11

## 本章讲什么

STL 容器不是铁板一块——`vector`/`deque`/`list`/`forward_list`/`map`/`set`/`unordered_*` 各有取舍：连续 vs 链式、有序 vs 哈希、单向 vs 双向。选错容器会让算法复杂度从 O(1) 变 O(n)。本章讲容器选型、元素拷贝代价、区间操作、指针容器内存管理与分配器。

---

## 各 Item 要点

### Item 1：仔细选择容器

| 需求 | 首选容器 |
|------|----------|
| 连续内存、随机访问、尾插 | `vector` |
| 头尾双端操作 | `deque` |
| 频繁中间插删 | `list` / `forward_list` |
| 有序键值查找 | `map` / `set`（红黑树 O(log n)） |
| 无序快速查找 | `unordered_map` / `unordered_set`（哈希 O(1) 均摊） |
| 小整数键密集 | `vector` 直接下标（比 map 快） |

**关键区分**：连续内存容器（`vector`/`deque`/`string`）cache 友好但中间插删 O(n)；节点容器（`list`/`map`）插删 O(1) 但指针追逐 cache 不友好。HFT 默认选 `vector` 换 cache 局部性。

### Item 2：不要试图写"容器无关"代码

序列容器与关联容器接口差异大（`[]` vs `find`、迭代器失效规则不同），想用 `typedef` 一行切换容器类型是不现实的。务实做法：用 `typedef` 固定一种容器，性能不达标再换，但接受接口改动的成本。

### Item 3：使容器里对象的拷贝轻量且正确

容器插入元素是**按值拷贝**（`push_back`/`insert` 拷贝一份）。元素拷贝代价 = 容器操作代价的底座。对策：存指针/智能指针（拷贝廉价）、用 `emplace` 直接构造、用移动语义（`move` 进容器）。

**拷贝的等价性问题**：`vector<Base>` 存 `Derived` 会发生**对象切片**——派生部分丢失。多态对象必须存指针（`vector<unique_ptr<Base>>`）。

### Item 4：用 `empty()` 而非 `size()==0`

`empty()` 对所有容器都是 O(1)；`size()` 对 `list` 在 C++11 前可能是 O(n)（需遍历计数）。虽然 C++11 起 `list::size()` 强制 O(1)，但 `empty()` 表意更清晰且无歧义。

### Item 5：优先区间成员函数

`assign(first, last)`、`insert(pos, first, last)`、`erase(first, last)` 等区间版本比单元素循环高效——区间版一次知道范围，可批量预留/移动；循环版每次单插，多次扩容 + 移动。

```cpp
v.insert(v.end(), src.begin(), src.end());   // 区间：一次操作
for (auto it = src.begin(); it != src.end(); ++it) v.push_back(*it);  // 多次扩容
```

### Item 6：警惕"最烦人解析"

`list<int> data(istream_iterator<int>(cin), istream_iterator<int>());` 被解析成**函数声明**（参数是函数指针）而非构造对象。加额外括号或用 C++11 `{}` 初始化规避。

### Item 7：容器销毁时删除指针

`vector<Widget*>` 析构不会 `delete` 指针——内存泄漏。用 `for_each + delete` 或直接存 `vector<unique_ptr<Widget>>` 让 RAII 自动释放。

### Item 8：不存 `auto_ptr`

`auto_ptr` 拷贝是"转移所有权"，STL 算法拷贝元素时会意外掏空原对象。C++11 起 `auto_ptr` 弃用，改用 `unique_ptr`（明确不可拷贝）或 `shared_ptr`。

### Item 9：删除元素的正确方式

删除满足条件的元素，`remove` + `erase` 是惯用法（见 ch5 Item 32-33）。但**指针容器**不能直接 `remove`——被跳过的指针会泄漏。要先用 `for_each` delete 再 `remove`，或存智能指针。

### Item 10–11：分配器

分配器（allocator）自定义容器内存来源。默认 `std::allocator` 走 `operator new`。HFT 用自定义分配器接 mempool / hugepage，但分配器是"相等性"敏感的——C++11 起要求无状态分配器（stateless），跨容器共享内存才安全。

---

## HFT 关联

- **`vector` 连续存储换 cache**：订单簿档位、tick 缓冲用 `vector`，顺序遍历 cache 命中率高；`map`/`list` 的指针追逐在每 tick 路径上引入 cache miss，延迟尖峰。
- **`reserve` 预留**：知道容量的 `vector`/`unordered_map` 预 `reserve`，避免热路径扩容 + rehash。HFT 启动时按峰值容量 reserve。
- **智能指针容器**：策略对象池用 `vector<shared_ptr<Strategy>>`，RAII 管理生命周期，避免裸指针容器的删除陷阱。
- **自定义分配器接 mempool**：高频分配的小对象（如订单节点）用 `std::list<T, MempoolAlloc<T>>` 接 mempool，避免 `operator new` 的锁与碎片。

---

## 自测题

1. 连续内存容器和节点容器在 cache 局部性上的根本区别是什么？HFT 默认选哪个？
2. `vector<Base>` 存 `Derived` 对象会发生什么？多态对象该怎么存？
3. 为什么区间成员函数 `insert(pos, first, last)` 比循环 `push_back` 高效？
4. `vector<Widget*>` 析构会释放 Widget 吗？怎么安全删除指针容器？
5. 指针容器为什么不能直接用 `remove` + `erase` 删除？正确做法是什么？
