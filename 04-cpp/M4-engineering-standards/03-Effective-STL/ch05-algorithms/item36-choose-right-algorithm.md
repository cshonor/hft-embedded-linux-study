# Item 36：正确选择算法

> 第 5 章 算法 · Item 36 · 上一节：[Item 35 算法优于循环](item35-algorithm-vs-loop.md) · 下一节：[Item 37 accumulate/for_each/fill](item37-accumulate-for-each-fill.md)

## 为什么要学这个（先建立直觉）

C 程序员只有 `qsort` + `bsearch` 两种算法。C++ STL 提供了几十种算法，选对算法能用 O(n) 解决 O(n log n) 的问题：

```cpp
// 只需要第 3 大的元素 → 不需要全排序！
std::nth_element(v.begin(), v.begin() + 2, v.end());  // O(n)
// vs
std::sort(v.begin(), v.end());  // O(n log n) → 杀鸡用牛刀
```

---

## 这节讲什么

`sort`/`stable_sort`/`partial_sort`/`nth_element` 各有适用场景。`for_each`/`transform`/`copy` 语义不同。按意图选算法，别混用。

---

## 排序算法选择

| 算法 | 复杂度 | 用途 | 保相对顺序 |
|------|--------|------|-----------|
| `sort` | O(n log n) | 全排序 | ❌ |
| `stable_sort` | O(n log²n) | 全排序 + 保相等元素顺序 | ✅ |
| `partial_sort` | O(n log k) | 只排前 k 个 | ❌ |
| `nth_element` | O(n) | 只保证第 n 大在位 | ❌ |

```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7, 4, 6};

// 全排序
std::sort(v.begin(), v.end());  // {1,2,3,4,5,6,7,8,9}

// 只排前 3 个
std::partial_sort(v.begin(), v.begin() + 3, v.end());  // {1,2,3,?,?,?,...}

// 只找第 4 大（中位数附近）
std::nth_element(v.begin(), v.begin() + 3, v.end());  // v[3] 是第 4 小
// 前面都 ≤ v[3]，后面都 ≥ v[3]，但不完全排序

// 保相等元素相对顺序
std::stable_sort(v.begin(), v.end());
```

### for_each vs transform vs copy

```cpp
// for_each：原地修改，不产生新区间
std::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });

// transform：产生新区间
std::transform(v.begin(), v.end(), std::back_inserter(result),
               [](int x) { return x * 2; });

// copy：仅搬运（不修改）
std::copy(v.begin(), v.end(), std::back_inserter(dst));
```

---

## 常见错误（新手踩坑）

### 错误 1：用 sort 找中位数

```cpp
std::sort(v.begin(), v.end());  // O(n log n)
int median = v[v.size() / 2];
// 杀鸡用牛刀——只需要第 n 大在位
```

**修正：** `std::nth_element(v.begin(), v.begin() + v.size()/2, v.end());` O(n)

### 错误 2：用 sort 只取前 k 个

```cpp
std::sort(v.begin(), v.end());  // O(n log n)
// 只取前 5 个
```

**修正：** `std::partial_sort(v.begin(), v.begin() + 5, v.end());` O(n log k)

### 错误 3：需要稳定排序时用 sort

```cpp
struct Order { double price; int timestamp; };
// 按价格排序，同价格按时间先后
std::sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) { return a.price < b.price; });
// 相同价格的 Order 相对顺序可能变！
```

**修正：** `std::stable_sort`（保相等元素的相对顺序）。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 排序 | `qsort` 一种 | `sort`/`stable_sort`/`partial_sort`/`nth_element` | 按需选择 |
| 部分排序 | 手写或 qsort 全排 | `partial_sort` O(n log k) | 更高效 |
| 选择第 n | qsort 全排 | `nth_element` O(n) | 更高效 |
| 稳定性 | `qsort` 不保证 | `stable_sort` 保证 | 保相对顺序 |

**一句话：** C 只有 `qsort`。C++ STL 提供四种排序——`sort`（全排）、`stable_sort`（稳定）、`partial_sort`（前 k 个）、`nth_element`（第 n 大）——按需选择，避免杀鸡用牛刀。

---

## HFT 关联

- **`nth_element` 求分位数**：回测里求 tick 序列的 P99 延迟，`nth_element` O(n) 比全排序 O(n log n) 快。
- **`partial_sort` 取 top-N**：找最优 N 个报价，`partial_sort` O(n log k) 比 `sort` O(n log n) 快。
- **`stable_sort` 订单簿**：订单按价格排序，同价格保时间先后——`stable_sort` 保证 FIFO 语义。

---

## 代码自测

### Q1: nth_element
```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7};
std::nth_element(v.begin(), v.begin() + 3, v.end());
std::cout << v[3];
```
> v[3] 是什么？

<details>
<summary>答案</summary>

v[3] = **5**（第 4 小的元素，即 7 个元素的中位数）。

`nth_element` 保证：v[3] 是排序后会在 index 3 位置的元素。前面都 ≤ v[3]，后面都 ≥ v[3]，但不完全排序。复杂度 O(n)。
</details>

### Q2: partial_sort
```cpp
std::vector<int> v = {5, 3, 8, 1, 9, 2, 7};
std::partial_sort(v.begin(), v.begin() + 3, v.end());
// v 的前 3 个元素是什么？
```

<details>
<summary>答案</summary>

v 的前 3 个 = `{1, 2, 3}`（最小的 3 个，已排序）。后面 `{8, 9, 5, 7}` 未排序。

`partial_sort` 排序前 k 个最小元素，后面不保证顺序。复杂度 O(n log k)。
</details>

### Q3: sort vs stable_sort
```cpp
struct Item { int val; int id; };
std::vector<Item> v = {{5,1}, {3,2}, {5,3}, {3,4}};

// A: sort
std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.val < b.val; });
// B: stable_sort
std::stable_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.val < b.val; });
```
> A 和 B 后 {5,1} 和 {5,3} 的相对顺序？

<detailf>
<summary>答案</summary>

- **A（sort）**：不保证。{5,1} 可能在 {5,3} 前或后。
- **B（stable_sort）**：保证 {5,1} 在 {5,3} 前（保持原始相对顺序）。

`stable_sort` 保持相等元素（`val` 相同）的原始相对顺序。需要 FIFO 语义（如订单按价格排序，同价格保时间先后）时用 `stable_sort`。
</details>

### Q4: 算法选择
```cpp
// 场景：100 万个 tick，找 P99 延迟
// A
std::sort(delays.begin(), delays.end());
double p99 = delays[delays.size() * 0.99];
// B
std::nth_element(delays.begin(), delays.begin() + delays.size() * 0.99, delays.end());
double p99 = delays[delays.size() * 0.99];
```
> A 和 B 哪个更高效？

<detailf>
<summary>答案</summary>

**B 更高效**。
- **A**：`sort` O(n log n) ≈ 100万 × 20 = 2000 万次比较。
- **B**：`nth_element` O(n) ≈ 100 万次比较。

B 快约 20 倍——因为你只需要第 99% 位置的元素，不需要完全排序。

**HFT 教训**：只求分位数/第 n 大时用 `nth_element`，不要全排序。
</details>

---

## 参考与延伸

- 上一节：[Item 35 算法优于循环](item35-algorithm-vs-loop.md)
- 下一节：[Item 37 accumulate/for_each/fill](item37-accumulate-for-each-fill.md)
- 回到：[第 5 章 算法](README.md)
