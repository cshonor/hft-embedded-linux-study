# 6.2 二分查找算法
> 第 6 章 算法 · 第 2 节 · 上一节：[6.1 内省排序](01-introsort.md) · 下一节：[6.3 copy 特化](03-copy-specialization.md)

## 为什么要学这个（先建立直觉）

C 的 `bsearch` 签名复杂、返回 `void*`、不区分 lower/upper：

```c
// C: bsearch 返回找到的任意一个匹配，不保证位置
int cmp(const void* a, const void* b) { return *(int*)a - *(int*)b; }
int* p = (int*)bsearch(&key, arr, n, sizeof(int), cmp);
// 找到了，但不知道是第一个还是最后一个匹配
// 想找范围？自己写二分
```

C++ 的 `lower_bound`/`upper_bound`/`equal_range` 精确定义查找语义：

```cpp
auto lo = std::lower_bound(v.begin(), v.end(), key);  // 第一个 >= key
auto hi = std::upper_bound(v.begin(), v.end(), key);  // 第一个 > key
// [lo, hi) 就是所有等于 key 的元素范围
```

理解这三个算法的返回值语义，是使用有序容器和有序数组的基础。

## 这节讲什么

STL 二分查找族在**有序区间**上工作，利用二分策略实现 O(log n) 查找。

### 三个核心算法

| 算法 | 返回 | 语义 | 场景 |
|------|------|------|------|
| `lower_bound` | 第一个 ≥ key 的位置 | "下界" | 找插入位置 |
| `upper_bound` | 第一个 > key 的位置 | "上界" | 找结束位置 |
| `equal_range` | `[lower, upper)` | 等值区间 | 找所有匹配 |
| `binary_search` | bool | 是否存在 | 只判断存在 |

### lower_bound 源码

```cpp
// SGI lower_bound 实现（简化）
template<class ForwardIterator, class T>
ForwardIterator lower_bound(ForwardIterator first,
                            ForwardIterator last, const T& value) {
    typedef typename iterator_traits<ForwardIterator>::difference_type Distance;
    Distance len = distance(first, last);  // 区间长度
    Distance half;
    ForwardIterator mid;

    while (len > 0) {
        half = len >> 1;           // 取中点
        mid = first;
        advance(mid, half);        // 移动到中点
        if (*mid < value) {        // 中点 < 目标 → 右半
            first = mid;
            ++first;
            len = len - half - 1;
        } else {                   // 中点 >= 目标 → 左半
            len = half;
        }
    }
    return first;  // 第一个 >= value 的位置
}
```

关键：`*mid < value` 时往右，`*mid >= value` 时往左。最终 `first` 指向第一个 ≥ value 的元素。

### upper_bound vs lower_bound

```cpp
// 有序数组: 1 3 5 5 5 7 9
//                ^lower_bound(5)  ^upper_bound(5)

// lower_bound(5)  → 指向第一个 5（第一个 >= 5）
// upper_bound(5)  → 指向 7（第一个 > 5）
// equal_range(5)  → [第一个5, 第一个7) = 所有 5 的范围
// binary_search(5) → true（存在 5）
```

### equal_range 源码

```cpp
// equal_range = lower_bound + upper_bound 的组合
// 但 SGI 实现更高效：一次二分同时缩小两端
template<class ForwardIterator, class T>
pair<ForwardIterator, ForwardIterator>
equal_range(ForwardIterator first, ForwardIterator last, const T& value) {
    // 二分缩小范围，直到 [first, last) 全等于 value 或空
    // 返回 {lower_bound, upper_bound}
}
```

### 关联容器的成员版本

```cpp
// map/set 的成员函数版本更高效（直接利用树结构）
std::map<int, string> m;
auto it = m.lower_bound(5);    // O(log n)，利用红黑树结构
auto range = m.equal_range(5); // O(log n)

// 全局算法版本不利用树结构，是 O(n)！
auto it = std::lower_bound(m.begin(), m.end(), 5);  // O(n)，别用！
```

关联容器的成员 `lower_bound` 利用红黑树结构 O(log n)，全局 `std::lower_bound` 对 `map` 迭代器是 O(n)（双向迭代器，advance 是线性）。

## 常见错误（新手踩坑）

### 错误 1：在无序区间上用 lower_bound

```cpp
// ❌ lower_bound 前提是区间有序
std::vector<int> v = {5, 1, 3, 7, 2};
auto it = std::lower_bound(v.begin(), v.end(), 3);  // 未定义行为
// 结果不可预测！
```

二分查找的前提是**有序区间**。无序必须先 sort，或用 `std::find`（O(n)）。

### 错误 2：对 map 用全局 lower_bound

```cpp
// ❌ 全局 std::lower_bound 对 map 是 O(n)
std::map<int, int> m;
// ... 填充 ...
auto it = std::lower_bound(m.begin(), m.end(), 5,
    [](auto& p, int v) { return p.first < v; });  // O(n)！

// 正确：用成员函数
auto it = m.lower_bound(5);  // O(log n)
```

### 错误 3：混淆 binary_search 和 find

```cpp
// ❌ binary_search 返回 bool，不是迭代器
bool found = std::binary_search(v.begin(), v.end(), 5);
// 只知道"存在"，不知道位置

// 要位置用 lower_bound
auto it = std::lower_bound(v.begin(), v.end(), 5);
if (it != v.end() && *it == 5) { /* 找到了 */ }
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| `bsearch` 返回任意匹配 | `lower_bound`/`upper_bound` 精确边界 | C++ 语义清晰 |
| `void*` 无类型安全 | 模板编译期检查 | C++ 安全 |
| 无 `equal_range` | `equal_range` 返回范围 | C++ 开箱即用 |
| 无成员版本 | 容器成员版本利用树结构 | C++ 更高效 |

## HFT 关联

- **有序价格档位查找**：`map<Price, Volume>` 用 `lower_bound(ask_price)` O(log n) 定位最优卖价，比线性扫描快
- **vector 替代 map**：小规模价格档位用 `vector<pair<Price, Volume>>` + sort + `lower_bound`，cache 友好
- **equal_range 统计**：`equal_range(price)` 获取同价所有订单，O(log n) 定位范围

## 代码自测

### Q1: lower_bound 和 upper_bound 返回值有什么区别？

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};
auto lo = std::lower_bound(v.begin(), v.end(), 5);  // ?
auto hi = std::upper_bound(v.begin(), v.end(), 5);  // ?
```
> lo 和 hi 分别指向哪个元素？

<details>
<summary>答案与复习指引</summary>

```
索引:  0  1  2  3  4  5  6
值:    1  3  5  5  5  7  9
             ^lo        ^hi
```

- `lo` 指向索引 2（第一个 ≥ 5 的元素，即第一个 5）
- `hi` 指向索引 5（第一个 > 5 的元素，即 7）
- `[lo, hi)` = 索引 2,3,4 = 所有等于 5 的元素

**口诀**：lower 是"下界"（第一个不小于），upper 是"上界"（第一个大于）。

**复习：** → [三个核心算法](./02-binary-search-algorithms.md)
</details>

### Q2: lower_bound 的二分逻辑是什么？

```cpp
while (len > 0) {
    half = len >> 1;
    mid = first + half;
    if (*mid < value) { first = mid + 1; len -= half + 1; }
    else              { len = half; }
}
return first;
```
> 为什么 `*mid < value` 用 `<` 而不是 `<=`？如果用 `<=` 会怎样？

<details>
<summary>答案与复习指引</summary>

**用 `<`**：`*mid < value` 为真时往右，`*mid >= value` 时往左。最终返回第一个 ≥ value 的位置（lower_bound 语义）。

**用 `<=`**：`*mid <= value` 为真时往右，`*mid > value` 时往左。最终返回第一个 > value 的位置（upper_bound 语义）。

所以 `<` 和 `<=` 的区别正好对应 lower_bound 和 upper_bound。

**为什么不能混用**：
- lower_bound 必须 `<`：保证找到第一个 ≥ value
- upper_bound 必须 `<=`（等价于 `!(value < *mid)`）：保证找到第一个 > value

**复习：** → [lower_bound 源码](./02-binary-search-algorithms.md)
</details>

### Q3: 下面的代码哪里有问题？

```cpp
std::set<int> s = {1, 3, 5, 7, 9};
auto it = std::lower_bound(s.begin(), s.end(), 5);
```
> 这和 `s.lower_bound(5)` 有什么区别？

<details>
<summary>答案与复习指引</summary>

**全局 `std::lower_bound`**：对 `set` 迭代器（双向迭代器）是 **O(n)**——`advance(mid, half)` 在双向迭代器上是线性移动。

**成员 `s.lower_bound(5)`**：直接利用红黑树结构，**O(log n)**。

**结论**：关联容器（set/map/multiset/multimap）永远用成员版本的 `lower_bound`/`upper_bound`/`equal_range`/`find`/`count`，不用全局算法版本。

**有序数组/vector** 用全局 `std::lower_bound`（随机访问迭代器，O(log n)）。

**HFT**：`map<Price, Vol>` 的 `lower_bound` 是 O(log n)，但如果价格档位少（< 100），`vector<pair>` + `std::lower_bound` 可能更快（cache 友好）。

**复习：** → [关联容器的成员版本](./02-binary-search-algorithms.md)
</details>

### Q4: equal_range 有什么用？

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};
auto [lo, hi] = std::equal_range(v.begin(), v.end(), 5);
int count = hi - lo;  // ?
```
> equal_range 返回什么？count 是多少？

<details>
<summary>答案与复习指引</summary>

**equal_range 返回**：`{lower_bound, upper_bound}` 组成的 pair，即 `[lo, hi)` 是所有等于 5 的元素范围。

**count = 3**（3 个 5）。

```cpp
// 等价于
auto lo = std::lower_bound(v.begin(), v.end(), 5);
auto hi = std::upper_bound(v.begin(), v.end(), 5);
int count = hi - lo;  // 3
```

但 `equal_range` 一次调用同时获取两端，比分别调用更高效（某些实现一次二分缩小两端）。

**HFT**：`equal_range(price)` 获取同价所有订单的数量，O(log n) 定位 + O(k) 遍历。

**复习：** → [equal_range](./02-binary-search-algorithms.md)
</details>

## 参考与延伸

- 上一节：[6.1 内省排序](01-introsort.md)
- 下一节：[6.3 copy 特化](03-copy-specialization.md)
- 源码参考：`bits/stl_algo.h`（`__lower_bound` / `__upper_bound`）
