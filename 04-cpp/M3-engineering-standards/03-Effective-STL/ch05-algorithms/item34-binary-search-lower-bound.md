# Item 34：排序后用 `binary_search` / `lower_bound`

> 第 5 章 算法 · Item 34 · 上一节：[Item 32-33 erase-remove 惯用法](item32-33-erase-remove-idiom.md) · 下一节：[Item 35 算法优于循环](item35-algorithm-vs-loop.md)

## 为什么要学这个（先建立直觉）

C 程序员在有序数组上二分查找：

```c
qsort(arr, n, sizeof(int), cmp);
// 然后 bsearch
int* found = bsearch(&key, arr, n, sizeof(int), cmp);
```

C++ 的 STL 提供了更丰富的二分查找工具：

```cpp
std::sort(v.begin(), v.end());
std::binary_search(v.begin(), v.end(), 42);  // 返回 bool
auto it = std::lower_bound(v.begin(), v.end(), 42);  // 第一个 ≥ 42
auto it2 = std::upper_bound(v.begin(), v.end(), 42);  // 第一个 > 42
auto [lo, hi] = std::equal_range(v.begin(), v.end(), 42);  // [lo, hi) = 所有 42
```

---

## 这节讲什么

有序区间上 `binary_search`/`lower_bound`/`upper_bound`/`equal_range` 是 O(log n)。`sort` + `binary_search` 比 `find`（O(n)）快——但要先排序。静态数据排序后二分；动态数据用 `set`/`unordered_set`。

---

## 四种二分查找

```cpp
std::vector<int> v = {1, 2, 2, 2, 3, 4, 5};  // 已排序

// binary_search：判断是否存在
bool found = std::binary_search(v.begin(), v.end(), 2);  // true

// lower_bound：第一个 ≥ 2 的位置
auto lo = std::lower_bound(v.begin(), v.end(), 2);  // 指向第一个 2

// upper_bound：第一个 > 2 的位置
auto hi = std::upper_bound(v.begin(), v.end(), 2);  // 指向 3

// equal_range：[lower_bound, upper_bound)
auto [lo2, hi2] = std::equal_range(v.begin(), v.end(), 2);
// [lo2, hi2) 包含所有等于 2 的元素
auto count = hi2 - lo2;  // 3（有 3 个 2）
```

图示：
```
v: [1, 2, 2, 2, 3, 4, 5]
       ^        ^
       lo       hi
       |---equal_range---|
       count = 3
```

---

## 常见错误（新手踩坑）

### 错误 1：在未排序的容器上用 binary_search

```cpp
std::vector<int> v = {3, 1, 4, 1, 5};
std::binary_search(v.begin(), v.end(), 4);  // UB！未排序
```

**修正：** 先 `std::sort(v.begin(), v.end());`

### 错误 2：用 binary_search 获取位置

```cpp
bool found = std::binary_search(v.begin(), v.end(), 42);
// found 只是 bool，没有位置信息
```

**修正：** 要位置用 `lower_bound` 或 `equal_range`。

### 错误 3：动态数据每次插入后重新排序

```cpp
v.push_back(42);
std::sort(v.begin(), v.end());  // O(n log n) 每次插入！
std::binary_search(v.begin(), v.end(), 42);  // O(log n)
// 总代价 O(n log n) >> O(log n)
```

**修正：** 动态数据用 `std::set`（自动有序，插入 O(log n)）或 `std::unordered_set`（O(1) 均摊）。

---

## 新手要点（和 C 的区别）

| 维度 | C `bsearch` | C++ STL | 为什么 |
|------|------------|---------|--------|
| 返回值 | 指针或 NULL | bool / iterator / range | 更灵活 |
| 范围查找 | 无 | `equal_range` | 一次获取区间 |
| 插入位置 | 手动算 | `lower_bound` | 自动 |
| 类型安全 | ❌（void*） | ✅（模板） | 编译期检查 |

**一句话：** C 的 `bsearch` 只返回指针。C++ 的 STL 提供四种二分查找——`binary_search`（判断存在）、`lower_bound`（插入位置）、`upper_bound`（上界）、`equal_range`（范围）——满足不同需求。

---

## HFT 关联

- **排序后二分查找**：静态数据（如交易所列表）排序后 `binary_search` O(log n)，比 `find` O(n) 快。
- **`nth_element` 求分位数**：回测里求 tick 序列的 P99 延迟，`nth_element` O(n) 比全排序 O(n log n) 快。
- **`equal_range` 统计**：在有序 tick 数据中找某价格区间内的所有 tick，`equal_range` 一次 O(log n)。

---

## 代码自测

### Q1: 四种二分查找
```cpp
std::vector<int> v = {1, 2, 2, 2, 3, 4, 5};
bool found = std::binary_search(v.begin(), v.end(), 2);  // A
auto lo = std::lower_bound(v.begin(), v.end(), 2);       // B
auto hi = std::upper_bound(v.begin(), v.end(), 2);       // C
auto [l, r] = std::equal_range(v.begin(), v.end(), 2);   // D
```
> A/B/C/D 的结果分别是什么？

<details>
<summary>答案</summary>

- **A**：`true`（2 存在）
- **B**：`lo` 指向第一个 2（index 1）
- **C**：`hi` 指向 3（index 4，第一个 > 2 的）
- **D**：`[l, r)` = [index 1, index 4)，包含 3 个 2，`r - l` = 3
</details>

### Q2: lower_bound 插入位置
```cpp
std::vector<int> v = {1, 3, 5, 7, 9};
auto it = std::lower_bound(v.begin(), v.end(), 6);
// it 指向哪个元素？
```

<details>
<summary>答案</summary>

`it` 指向 **7**（index 3）。`lower_bound` 返回第一个 ≥ 6 的元素位置。如果在此位置插入 6，序列保持有序：

```cpp
v.insert(it, 6);  // v = {1, 3, 5, 6, 7, 9}
```
</details>

### Q3: 未排序的后果
```cpp
std::vector<int> v = {5, 3, 8, 1, 9};
bool found = std::binary_search(v.begin(), v.end(), 8);
// found 一定是 true 吗？
```

<detailf>
<summary>答案</summary>

**不一定**。`binary_search` 要求区间已排序。未排序的区间上调用是**未定义行为**——可能返回 true 也可能返回 false，取决于实现和内存布局。

**修正：** 先 `std::sort(v.begin(), v.end());` 再 `binary_search`。
</details>

### Q4: 静态 vs 动态
```cpp
// 场景：频繁查找，偶尔插入
// A: vector + sort + binary_search
// B: set（自动有序）
// C: unordered_set（哈希）
```
> 各方案的查找/插入复杂度？

<detailf>
<summary>答案</summary>

| 方案 | 查找 | 插入 | 适用场景 |
|------|------|------|---------|
| `vector`+sort+binary_search | O(log n) | O(n) 排序 or O(n) 插入+O(log n) 查找位置 | 静态数据 |
| `set` | O(log n) | O(log n) | 动态+需要有序 |
| `unordered_set` | O(1) 均摊 | O(1) 均摊 | 动态+不需要有序 |

**HFT 选择**：静态数据（如交易所列表）用 `vector` + `binary_search`（cache 友好）；动态数据用 `unordered_set`（O(1)）+ `reserve`（避免 rehash）。
</details>

---

## 参考与延伸

- 上一节：[Item 32-33 erase-remove 惯用法](item32-33-erase-remove-idiom.md)
- 下一节：[Item 35 算法优于循环](item35-algorithm-vs-loop.md)
- 回到：[第 5 章 算法](README.md)
