# 第 3 章 关联容器

**Associative Containers** — Items 18–25

## 本章讲什么

关联容器（`map`/`set`/`multimap`/`multiset` 及其 `unordered_` 版本）的核心是"按键查找"。本章讲清两个易混概念——**相等（equality）**与**等价（equivalence）**、`operator[]` 与 `insert` 的取舍、哈希容器的选择与调优。

---

## 各 Item 要点

### Item 18–19：理解相等与等价的区别

- **相等**：用 `operator==` 判断两个对象值是否相同。
- **等价**：关联容器用比较谓词（默认 `operator<`）判断 `!(a<b) && !(b<a)`——二者都不小于对方即"等价"。

关联容器用**等价**而非相等决定键的唯一性。这意味着：`operator==` 与 `operator<` 不一致时，容器认为"等价"的两个对象可能 `==` 为 false。这是 subtle bug 来源——自定义类型要保证 `<` 与 `==` 语义自洽。

### Item 20：为关联容器指定比较类型

`set<int>` 的第三个模板参数是比较**类型**而非函数：

```cpp
struct PtrCmp { bool operator()(int* a, int* b) const { return *a < *b; } };
std::set<int*, PtrCmp> s;   // 按指针所指值排序
```

比较类型是无状态的函数对象（stateless functor）。Lambda 不能直接作类型参数（无类型名），要用 `decltype` + 传构造参数，或写 struct。

### Item 21：`map`/`set` 的键是 `const`

`map<K,V>::value_type` 是 `pair<const K, V>`——键不可修改。想改键只能删旧插新。这是为了维护容器的有序/哈希不变量。

### Item 22：`operator[]` vs `insert` 的取舍

- `m[k]`：键不存在则**默认构造**值并插入，返回引用；存在则返回引用。适合"取值或默认"。
- `m.insert({k, v})`：不默认构造已有键的值，适合"插入新键"。

`m[k]` 的默认构造代价可能很高（如 `value_type` 有重构造）。仅查找不插入应 `find`，避免 `[]` 的默认构造副作用。

### Item 23：降序 `set` / `map`

用 `greater<K>` 作比较类型得到降序容器：`set<int, greater<int>>`。查找/遍历顺序随之反转。

### Item 24：`map::insert` 效率 vs `operator[]`

更新已有键的值，`m[k] = v` 与 `m.insert_or_assign(k, v)`（C++17）效率接近；插入新键 `insert` 略优（不构造默认值）。批量插入用 `insert(first, last)` 区间版。

### Item 25：哈希容器（`unordered_*`）的选择

`unordered_map`/`unordered_set`（C++11）基于哈希表，均摊 O(1) 查找。关键选择：
- **哈希函数**：默认 `std::hash<T>`，自定义类型需特化。
- **相等判断**：用 `operator==`（注意：哈希容器用**相等**，有序容器用**等价**！）。
- **桶数与负载因子**：`load_factor()` > `max_load_factor()` 时 rehash。预 `reserve(bucket_count)` 避免 rehash 尖峰。

**HFT**：`unordered_map` 适合"按键 O(1) 查找"，但哈希表 cache 不友好（桶链表指针追逐）。键密集且范围已知时，`vector` 直接下标更快。

---

## HFT 关联

- **`unordered_map` 的 rehash 尖峰**：订单 ID → 订单对象的映射，启动时按峰值 `reserve`，避免热路径 rehash 导致延迟尖峰。
- **等价 vs 相等 bug**：自定义订单键的 `operator<` 与 `operator==` 不一致，会导致 `set` 里出现"等价但不相等"的重复——订单去重失效。务必自洽。
- **`[]` 的默认构造副作用**：`m[orderId].qty` 在订单不存在时默认构造一个空订单——可能导致脏数据。查找用 `find`，插入用 `insert`/`try_emplace`（C++17）。
- **`vector` 替代 `map`**：小整数键（如交易所 ID 0-7）用 `vector<Exchange>` 直接下标，比 `map<int,Exchange>` 快一个数量级。

---

## 自测题

1. "等价"和"相等"的定义分别是什么？关联容器用哪个判断键唯一性？
2. 为什么 `map` 的键是 `const`？想改键怎么办？
3. `m[k]` 和 `m.find(k)` 在键不存在时行为有何不同？为什么热路径要避免 `[]` 误用？
4. `unordered_map` 用相等还是等价？rehash 什么时候发生？怎么避免？
5. 小整数键密集时，为什么 `vector` 比 `map` 更适合？

## 代码自测

### Q1: map vs unordered_map
```cpp
// 场景：symbol → price 映射，5000 个 symbol
std::map<std::string, double> m;        // A: 红黑树
std::unordered_map<std::string, double> um;  // B: 哈希表

// 查找
m.find("AAPL");     // O(log n)
um.find("AAPL");    // O(1) 均摊
```
> 除了查找复杂度，两者还有哪些关键区别？

<details>
<summary>答案与复习指引</summary>

| 特性 | `map` | `unordered_map` |
|------|-------|----------------|
| 底层结构 | 红黑树 | 哈希表（桶数组） |
| 有序性 | ✅ 按 key 排序 | ❌ 无序 |
| 最坏查找 | O(log n) | O(n)（哈希冲突） |
| 迭代器失效 | 插入不失效，删除只失效被删元素 | rehash 时全部失效 |
| 内存 | 每节点 3 指针 | 桶数组 + 链表节点 |
| cache | ❌ 指针追逐 | 略好但仍非连续 |

**HFT 选择**：如果不需要有序遍历，选 `unordered_map` + `reserve` 预分配避免 rehash。需要有序遍历（如按 symbol 排序输出）选 `map`。

**复习：** → [关联容器选型](./README.md)
</details>

### Q2: operator[] 的副作用
```cpp
std::map<std::string, int> m;
// 查找 "AAPL"，不存在时不要插入
int price = m["AAPL"];  // A: 有问题
auto it = m.find("AAPL");  // B: 正确
```
> A 行有什么副作用？为什么在 HFT 中要避免？

<details>
<summary>答案与复习指引</summary>

**A 行副作用**：`operator[]` 在 key 不存在时**自动插入**默认值（int → 0）。所以 `m["AAPL"]` 会创建一个 `{AAPL: 0}` 条目，即使你只想查找。

**问题**：
1. 意外修改容器（非 const 操作）
2. 插入开销（红黑树再平衡）
3. 语义不清晰（查找 vs 插入混淆）

**正确做法**：`find` + 检查 `end()`，或 C++20 `contains`：
```cpp
if (m.contains("AAPL")) { ... }
```

**复习：** → [operator[] 副作用](./README.md)
</details>
