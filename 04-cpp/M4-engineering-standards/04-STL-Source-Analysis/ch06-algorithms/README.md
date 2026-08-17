# 第 6 章 算法

**Algorithms**

## 本章讲什么

STL 算法分**质变算法**（修改区间，如 `sort`/`copy`/`remove`）与**非质变算法**（只读，如 `find`/`count`/`accumulate`）。本章剖析几个关键算法的源码实现：内省排序、二分查找、`copy` 的特化优化——理解这些才能预测性能。

## 要点

### `sort`：内省排序（introsort）

SGI `sort` 是快排 + 堆排 + 插入排序的混合：
1. 快排递归分段；
2. 递归深度超阈值（2·log n）切到堆排（保证最坏 O(n log n)，防快排退化 O(n²)）；
3. 段小于阈值（如 16）切到插入排序（小数据常数更优）。

这就是"内省"——发现快排退化时自省切堆排。`list::sort` 则是归并（链表无随机访问，不能用内省排序）。

### `lower_bound`/`upper_bound`：二分查找

有序区间上 O(log n)。`lower_bound` 返回第一个"不小于"目标的位置；`upper_bound` 返回第一个"大于"目标的位置。`equal_range` = 二者组成的等值区间。

### `copy` 的特化

`copy` 对 trivially copyable 类型 + 随机访问迭代器特化为 `memmove`——比逐元素赋值快数倍。这是 traits 萃取 `is_trivially_copyable` + `iterator_category` 联合分派的成果，编译期零开销选最优路径。

### `accumulate` / `for_each`

`accumulate` 默认求和，可挂自定义二元操作。`for_each` 遍历执行，返回仿函数副本（取最终状态）。二者都是非质变（除非仿函数修改元素）。

## HFT 关联

- **`copy` 的 `memmove` 特化**：批量拷贝 tick 数据用 `std::copy` 比手写循环更可能走 `memmove`/SIMD——编译器对 traits 特化的优化比手写循环更激进。
- **`sort` 的最坏保证**：内省排序保证 O(n log n)，HFT 回测排序延迟分布可预测；但热路径避免动态排序（预排序或桶）。
- **`lower_bound` 二分**：有序档位查找用 `lower_bound` O(log n)，比线性 `find` 快。

## 自测题

1. 内省排序如何保证最坏 O(n log n)？它在什么条件下切到堆排和插入排序？
2. `list::sort` 为什么不能用 `std::sort`？它用什么算法？
3. `copy` 在什么条件下特化为 `memmove`？traits 如何参与这个决策？
4. `lower_bound` 和 `upper_bound` 的返回位置有何不同？
5. `accumulate` 的初值类型如何影响结果类型？HFT 为什么要用 `0LL`？

## 代码自测

### Q1: sort 的内省排序
```cpp
// std::sort 的内部策略（简化）
template<typename Iter>
void sort(Iter first, Iter last) {
    // 1. 快速排序（pivot = median-of-three）
    // 2. 递归深度超过 2*log(n) → 切换到堆排序（防最坏 O(n²)）
    // 3. 区间 < 16 → 切换到插入排序（小数据常数因子低）
}
```
> 为什么要混合三种排序？各有什么优劣？

<details>
<summary>答案与复习指引</summary>

**内省排序（Introsort）** 的三阶段策略：

| 排序 | 平均 | 最坏 | 优势 | 劣势 |
|------|------|------|------|------|
| 快排 | O(n log n) | O(n²) | 常数因子最小 | 恶化（已排序/特殊 pivot） |
| 堆排 | O(n log n) | O(n log n) | 保证最坏复杂度 | 常数因子大、cache 不友好 |
| 插入排序 | O(n²) | O(n²) | 小数据极快、几乎无开销 | 大数据退化 |

**混合策略**：
1. 先用快排（平均最快）
2. 递归太深（可能恶化）→ 切堆排（保证 O(n log n)）
3. 区间足够小 → 切插入排序（小数据常数因子低）

**HFT**：了解 sort 内部机制有助于预测性能。热路径避免对已排序数据 sort（虽然内省排序防恶化但仍有开销），或用 `is_sorted` 先检查。

**复习：** → [sort 内部机制](./README.md)
</details>
