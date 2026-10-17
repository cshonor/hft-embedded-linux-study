# 6.1 内省排序（Introsort）
> 第 6 章 算法 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[6.2 二分查找算法](02-binary-search-algorithms.md)

## 为什么要学这个（先建立直觉）

C 的 `qsort` 是纯快速排序，最坏情况退化到 O(n²)：

```c
// C: qsort 最坏 O(n²)，无保障
int cmp(const void* a, const void* b) { return *(int*)a - *(int*)b; }
qsort(arr, n, sizeof(int), cmp);
// 对已排序数组或特殊 pivot，qsort 退化 O(n²)
// 函数指针阻碍内联，常数因子大
```

C++ 的 `std::sort` 用**内省排序**——快排 + 堆排 + 插入排序的混合体，保证最坏 O(n log n)：

```cpp
std::sort(v.begin(), v.end());  // 保证 O(n log n)，模板可内联
```

理解内省排序的三阶段策略，你才能理解 `std::sort` 为什么既快又有保障。

## 这节讲什么

SGI `sort` 的核心是"自省"——递归深度过大时放弃快排切堆排，保证最坏复杂度。

### 三阶段策略

```
std::sort(first, last)
  │
  ├─ 区间 < 16 → 插入排序（小数据常数因子低）
  │
  ├─ 递归深度 > 2·log(n) → 堆排序（防快排退化）
  │
  └─ 否则 → 快速排序（median-of-three pivot）
```

### 阶段 1：快速排序

```cpp
// pivot 选择：三数取中（median-of-three）
Iter mid = first + (last - first) / 2;
// 比较 first, mid, last-1，取中间值做 pivot
// 避免已排序/逆序数组退化

// 分割：小于 pivot 左边，大于 pivot 右边
// 然后对左右子段递归
```

快排平均 O(n log n)，常数因子最小，但对特殊输入可能退化 O(n²)。

### 阶段 2：堆排序（兜底）

```cpp
// 递归深度超过阈值时切换
if (depth_limit == 0) {
    std::partial_sort(first, last, last);  // 内部用堆排序
    return;
}
--depth_limit;
// depth_limit = 2 * log2(last - first)
```

堆排序最坏 O(n log n)，保证不退化。代价是常数因子大、cache 不友好，所以只在快排可能退化时才用。

### 阶段 3：插入排序（收尾）

```cpp
// 最终对整个区间做一次插入排序
// 此时区间已"大致有序"（每段 < 16 已部分排序）
// 插入排序对近似有序数据接近 O(n)
__final_insertion_sort(first, last);
```

小数据（< 16 元素）插入排序比快排更快——没有递归开销、分支预测友好。

### 三种排序的对比

| 排序 | 平均 | 最坏 | 优势 | 劣势 |
|------|------|------|------|------|
| 快排 | O(n log n) | O(n²) | 常数最小、cache 友好 | 退化风险 |
| 堆排 | O(n log n) | O(n log n) | 最坏有保证 | 常数大、cache 不友好 |
| 插入 | O(n²) | O(n²) | 小数据极快、近似有序 O(n) | 大数据退化 |

混合策略取三者之长：快排（平均快）+ 堆排（最坏保证）+ 插入（小数据快）。

## 常见错误（新手踩坑）

### 错误 1：对 list 用 std::sort

```cpp
// ❌ std::sort 需要随机访问迭代器
std::list<int> l = {3, 1, 4, 1, 5};
std::sort(l.begin(), l.end());  // 编译错误！
// list 是双向迭代器，不支持随机访问
```

`list` 必须用成员函数 `l.sort()`，它用归并排序（链表天然适合归并）。

### 错误 2：以为 sort 是稳定的

```cpp
// ❌ std::sort 不保证稳定
struct Order { int price; int seq; };
std::vector<Order> v = {{100, 1}, {100, 2}, {50, 3}};
std::sort(v.begin(), v.end(),
    [](auto& a, auto& b) { return a.price < b.price; });
// {100,1} 和 {100,2} 的相对顺序不保证
// 需要 stable_sort 才保证
```

需要稳定排序用 `std::stable_sort`（归并排序，O(n log n) 额外空间）。

### 错误 3：对已排序数据反复 sort

```cpp
// ❌ 不必要的排序（浪费 CPU）
if (!std::is_sorted(v.begin(), v.end()))
    std::sort(v.begin(), v.end());
```

虽然内省排序防退化，但对已排序数据仍有 O(n) 的检查 + 分割开销。更好的做法是保持有序（插入时用 `lower_bound` 定位）。

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| `qsort` 纯快排，最坏 O(n²) | `std::sort` 内省排序，最坏 O(n log n) | C++ 有保障 |
| 函数指针，无法内联 | 模板/lambda，可内联 | C++ 常数更小 |
| 无类型安全（`void*`） | 模板编译期检查 | C++ 安全 |
| 无 `is_sorted` 检查 | `std::is_sorted` 可先检查 | C++ 更灵活 |

## HFT 关联

- **最坏延迟可预测**：内省排序保证 O(n log n)，回测排序延迟有上界——不像 `qsort` 可能 O(n²) 爆炸
- **热路径避免动态排序**：即使 O(n log n)，热路径也不排序——预排序或用桶
- **lambda 可内联**：`std::sort(v.begin(), v.end(), [](auto a, auto b){...})` 的 lambda 编译器可内联，比函数指针快
- **`nth_element` 替代**：只需 top-K 不需全排序时，用 `std::nth_element` O(n)，比 sort O(n log n) 快

## 代码自测

### Q1: 内省排序为什么要混合三种排序？

```cpp
// std::sort 内部逻辑（简化）
void sort(Iter first, Iter last, size_t depth_limit) {
    if (last - first < 16) return;  // 留给最终插入排序
    if (depth_limit == 0) return partial_sort(first, last, last);  // 切堆排
    // 否则快排分割 + 递归
}
// 最后统一做一次插入排序
```
> 每种排序负责什么角色？为什么不能只用一种？

<details>
<summary>答案与复习指引</summary>

| 排序 | 角色 | 不能只用的原因 |
|------|------|--------------|
| 快排 | 主力（平均最快） | 最坏 O(n²)，退化风险 |
| 堆排 | 兜底（最坏保证） | 常数大、cache 不友好，日常比快排慢 |
| 插入 | 收尾（小数据最快） | 大数据 O(n²) 退化 |

**混合逻辑**：
1. 大数据用快排（平均最快）
2. 快排递归太深（可能退化）→ 切堆排（保底 O(n log n)）
3. 数据足够小（< 16）→ 切插入排序（常数因子低）

单独用任何一种都有致命弱点，混合取长补短。

**复习：** → [三阶段策略](./01-introsort.md)
</details>

### Q2: median-of-three 如何避免退化？

```cpp
// 三数取中：比较 first, mid, last-1，取中间值做 pivot
Iter pivot = median_of_three(first, first + (last-first)/2, last-1);
```
> 如果不用 median-of-three，对已排序数组会发生什么？

<details>
<summary>答案与复习指引</summary>

**不用 median-of-three**（取首元素做 pivot）：
- 已排序数组：每次分割退化（pivot 是最小值），递归深度 n → O(n²)
- 逆序数组：同理退化

**用 median-of-three**：
- 已排序数组：mid 恰好是中位数，完美分割 → O(n log n)
- 逆序数组：mid 也接近中位数 → O(n log n)

median-of-three 不能防止所有退化（存在特殊构造的"杀手序列"），但能防最常见的已排序/逆序退化。真正的兜底是深度阈值切堆排。

**复习：** → [阶段 1 快速排序](./01-introsort.md)
</details>

### Q3: list::sort 为什么不能用 std::sort？

```cpp
std::list<int> l = {5, 3, 1, 4, 2};
// std::sort(l.begin(), l.end());  // 编译错误
l.sort();  // OK，用归并排序
```
> list 迭代器是什么类型？为什么不能快排？

<details>
<summary>答案与复习指引</summary>

**list 迭代器是双向迭代器**（BidirectionalIterator），只支持 `++`/`--`，不支持 `+n`/`[]`（随机访问）。

快排需要随机访问（取 mid = first + n/2，交换首尾），双向迭代器做不到。

**list::sort 用归并排序**：
- 链表天然适合归并（合并两个有序链表只需改指针，无需额外空间）
- 归并排序不需要随机访问
- 稳定排序（保持相等元素的相对顺序）
- O(n log n)

**HFT**：list 的归并排序虽然 O(n log n)，但链表 cache 不友好，实际比 vector 的内省排序慢。热路径用 vector + sort。

**复习：** → [迭代器分类](../ch03-iterators-traits/01-iterator-categories.md)
</details>

### Q4: stable_sort 和 sort 的区别？

```cpp
std::vector<std::pair<int, int>> v = {{100, 1}, {50, 2}, {100, 3}, {50, 4}};
// sort: {50,2}/{50,4} 顺序不保证, {100,1}/{100,3} 顺序不保证
std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });

// stable_sort: {50,2},{50,4},{100,1},{100,3} —— 相同 key 保持原顺序
std::stable_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });
```
> 什么场景需要 stable_sort？代价是什么？

<details>
<summary>答案与复习指引</summary>

**需要 stable_sort 的场景**：
- 多次排序：先按次要键排序，再按主要键稳定排序 → 最终主要键有序，次要键保持
- 需要保持原始插入顺序的相同元素

**代价**：
- `sort`：内省排序，O(n log n)，通常原地（O(1) 额外空间）
- `stable_sort`：归并排序，O(n log n)，但需要 O(n) 额外空间（或 O(log n) 如果内存不足）

**HFT**：订单按时间戳排序后按价格稳定排序 → 同价格按时间先后排列。

**复习：** → [sort 稳定性](./01-introsort.md)
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[6.2 二分查找算法](02-binary-search-algorithms.md)
- 源码参考：`bits/stl_algo.h`（GCC libstdc++ 的 `__introsort_loop` / `__final_insertion_sort`）
