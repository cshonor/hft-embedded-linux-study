# Item 47：避免直接修改算法的源码假设

> 第 7 章 使用 STL 编程 · Item 47 · 上一节：[Item 46 成员 vs 算法](item46-member-vs-algorithm.md) · 下一节：[Item 48 include 大小写](item48-include-case-sensitivity.md)

## 为什么要学这个（先建立直觉）

在 C 里，`qsort` 的实现通常是快速排序，但标准不保证——它只保证 O(n log n) 平均复杂度。如果你依赖"qsort 一定是快排"这个假设写代码（比如依赖 partition 的顺序），换编译器就出错。

```c
/* C: qsort 的实现是未指定的 */
// 标准只保证：qsort 排序后数组按升序排列
// 不保证用什么算法（快排？归并？堆排？）
// 不保证相等元素的相对顺序（不稳定）
```

```cpp
// C++: std::sort 同理
// 标准保证：O(n log n) 复杂度，排序后有序
// 不保证：用快排还是内省排序，相等元素顺序（不稳定）
// 不保证：具体比较次数

// 如果你依赖 sort 的内部实现行为，代码不可移植
```

**直觉**：只依赖标准保证的接口契约（复杂度、副作用、迭代器要求），不依赖具体实现。标准库的实现细节在不同编译器/版本上不同。

## 这节讲什么

### 标准保证 vs 实现细节

| 方面 | 标准保证（可依赖） | 实现细节（不可依赖） |
|------|-------------------|---------------------|
| `sort` 复杂度 | O(n log n) | 用快排/内省/pdqsort |
| `sort` 稳定性 | 不稳定 | 相等元素可能保持/不保持顺序 |
| `find` 复杂度 | O(n) | 具体比较次数 |
| `vector` 内存布局 | 连续 | 扩容因子 1.5 或 2 |
| `string` SSO | 无标准要求 | SSO 缓冲区大小 |
| `unordered_map` | O(1) 平均 | 哈希函数、桶数 |

### 不稳定排序的陷阱

```cpp
struct Item {
    int priority;
    int sequence;  // 插入顺序
};

std::vector<Item> v = {{1, 0}, {2, 1}, {1, 2}, {3, 3}, {1, 4}};
// 按 priority 排序
std::sort(v.begin(), v.end(),
    [](const Item& a, const Item& b) { return a.priority < b.priority; });

// 结果可能是：
// {1,0}, {1,2}, {1,4}, {2,1}, {3,3}  ← 保持顺序
// 或
// {1,4}, {1,2}, {1,0}, {2,1}, {3,3}  ← 乱序
// std::sort 不保证！
```

**修复**：需要稳定排序用 `std::stable_sort`。

### 不要依赖 vector 扩容因子

```cpp
std::vector<int> v;
v.reserve(10);
for (int i = 0; i < 10; i++) v.push_back(i);
v.push_back(10);  // 触发扩容
// v.capacity() 可能是 15（GCC: 1.5x）或 20（MSVC: 2x）
// 不要依赖具体的扩容因子！
```

### 不要依赖 unordered_map 迭代顺序

```cpp
std::unordered_map<int, std::string> m = {{1,"a"}, {2,"b"}, {3,"c"}};
for (auto& [k, v] : m) {
    std::cout << k << " ";  // 顺序未定义！
}
// 可能输出 1 2 3，也可能 3 1 2，每次运行可能不同
```

## 常见错误（新手踩坑）

### 错误 1：依赖 sort 稳定性

```cpp
std::sort(v.begin(), v.end(), cmp);  // 期望相等元素保持原顺序
// 但 sort 不稳定！
```

**修复**：用 `std::stable_sort`。

### 错误 2：依赖 unordered_map 遍历顺序

```cpp
std::unordered_map<int, int> m;
// ... 填充 ...
// 期望按插入顺序遍历 ← 错！
for (auto& [k, v] : m) { /* ... */ }
```

**修复**：需要顺序用 `std::map`（排序）或自维护插入顺序链表。

### 错误 3：依赖 vector 扩容因子做"优化"

```cpp
// 假设扩容因子是 2，预留正好够的数量
std::vector<int> v;
v.reserve(100);
// push_back 100 个元素后，capacity 正好 100
// 第 101 个 → 扩容到 200（假设 2x）
// 但别的编译器扩容 1.5x → 150
// 依赖这个做内存计算 → 不可移植
```

**修复**：`reserve` 到确定不会超的量，不依赖扩容因子。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| qsort/sort 算法 | 未指定 | 未指定（但有 stable_sort 选项） |
| 扩容机制 | malloc 手动管理 | vector 自动（因子未指定） |
| 哈希表 | 无标准 | unordered_map（顺序未指定） |
| 稳定性 | qsort 不稳定 | sort 不稳定，stable_sort 稳定 |

## HFT 关联

- **reserve 消除扩容**：不依赖扩容因子，而是 reserve 到精确需求量
- **stable_sort 保证顺序**：订单排序按时间+优先级，需要稳定排序
- **unordered_map 不做顺序依赖**：遍历顺序不可预测，需要顺序用 map 或自维护

## 代码自测

### Q1: sort 稳定性

```cpp
struct Order {
    int price;
    int timestamp;
};
std::vector<Order> orders = {{100, 1}, {100, 2}, {100, 3}};
std::sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) { return a.price < b.price; });
// orders 的 timestamp 顺序一定是 1, 2, 3 吗？
```

<details>
<summary>答案</summary>

**不一定**。`std::sort` 不保证稳定性——相等元素的相对顺序可能改变。

如果 price 相同，timestamp 可能变成 3, 1, 2 或任意排列。

**修复**：用 `std::stable_sort` 保持原始顺序。

```cpp
std::stable_sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) { return a.price < b.price; });
// 现在 price 相同时 timestamp 保持 1, 2, 3
```

或在比较器中加入 timestamp 作为 tiebreaker：
```cpp
[](const Order& a, const Order& b) {
    if (a.price != b.price) return a.price < b.price;
    return a.timestamp < b.timestamp;  // tiebreak
}
```
</details>

### Q2: unordered_map 顺序

```cpp
std::unordered_map<int, int> m;
m[3] = 30;
m[1] = 10;
m[2] = 20;
for (auto& [k, v] : m) std::cout << k << " ";
```
> 输出一定是 "3 1 2" 吗？

<details>
<summary>答案</summary>

**不一定**。`unordered_map` 的遍历顺序由哈希函数和桶结构决定，不保证插入顺序。

输出可能是 `1 2 3`、`3 1 2`、`2 3 1` 等——不同编译器/不同运行可能不同。

**需要顺序的方案**：
1. 用 `std::map`（按 key 排序）
2. 维护插入顺序的辅助 `std::vector<int> keys`
3. 用第三方有序哈希表（如 `boost::flat_map` 等）
</details>

### Q3: vector capacity

```cpp
std::vector<int> v;
v.reserve(100);
for (int i = 0; i < 100; i++) v.push_back(i);
std::cout << v.capacity();  // 一定是 100 吗？
```

<details>
<summary>答案</summary>

**是的，一定是 100**。`reserve(100)` 保证 capacity 至少 100。在 push_back 100 个元素后，没有触发扩容，capacity 保持 100（或更大，但不会因为 reserve 后 push_back 等量元素而扩容）。

但如果 push_back 第 101 个元素：
```cpp
v.push_back(100);  // 触发扩容
// 新 capacity 可能是 150（GCC 1.5x）或 200（MSVC 2x）
```

扩容因子是**实现定义**的，不要依赖具体值。
</details>

### Q4: 算法复杂度保证

```cpp
std::vector<int> v(1000000);
// A:
std::sort(v.begin(), v.end());
// B:
std::stable_sort(v.begin(), v.end());
// C:
std::nth_element(v.begin(), v.begin() + 500000, v.end());
```
> A、B、C 的复杂度保证分别是什么？

<details>
<summary>答案</summary>

- **A `std::sort`**：O(n log n) — 不稳定
- **B `std::stable_sort`**：O(n log²n) 或 O(n log n)（如果有额外内存）— 稳定
- **C `std::nth_element`**：O(n) 平均 — 只保证第 n 大的元素在正确位置

**HFT 用 nth_element 求 P50/P99**：
```cpp
// P99 延迟（不需要完全排序）
std::nth_element(latencies.begin(),
                 latencies.begin() + latencies.size() * 99 / 100,
                 latencies.end());
double p99 = latencies[latencies.size() * 99 / 100];
```

比 `sort` + 索引更高效（O(n) vs O(n log n)）。
</details>

## 参考与延伸

- 上一节：[Item 46 成员 vs 算法](item46-member-vs-algorithm.md)
- 下一节：[Item 48 include 大小写](item48-include-case-sensitivity.md)
