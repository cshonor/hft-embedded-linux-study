# 并行算法清单与语义约束

## 有并行版本的算法

C++17 给几乎所有 `<algorithm>` 加了执行策略重载：

### 非修改序列操作
- `for_each`、`for_each_n`
- `find`、`find_if`、`find_if_not`
- `find_end`、`find_first_of`
- `count`、`count_if`
- `search`、`search_n`
- `all_of`、`any_of`、`none_of`
- `equal`、`mismatch`

### 修改序列操作
- `copy`、`copy_n`、`copy_if`
- `move`
- `fill`、`fill_n`
- `transform`
- `generate`、`generate_n`
- `remove`、`remove_if`
- `unique`
- `reverse`、`rotate`
- `swap_ranges`

### 排序与分区
- `sort`、`stable_sort`
- `partial_sort`、`partial_sort_copy`
- `nth_element`
- `partition`、`stable_partition`

### 合并与二分
- `merge`
- `inplace_merge`
- `lower_bound`、`upper_bound`
- `equal_range`
- `binary_search`

### 数值操作（`<numeric>`）
- `reduce`
- `transform_reduce`
- `inclusive_scan`、`exclusive_scan`
- `transform_inclusive_scan`、`transform_exclusive_scan`

### 最小/最大
- `min_element`、`max_element`、`minmax_element`

## 语义约束差异

| 算法 | 并行约束 |
|------|----------|
| `for_each` | 不保证调用顺序；函数对象不能修改序列长度 |
| `find(par)` | 找到后**不保证立即停止**——其他线程可能已在处理后续元素 |
| `reduce` | 操作须满足**结合律**（不要求交换律）→ 浮点结果可能不同 |
| `sort(par)` | 复杂度仍 O(n log n)，但比较次数可能更多 |
| `fill`/`copy` | 并行写入不重叠区域，安全 |
| `partition(par)` | 相对顺序不保证（用 `stable_partition` 如果需要） |

## find 的并行陷阱

```cpp
// 串行：找到第一个就停
auto it = std::find(v.begin(), v.end(), target);

// 并行：不保证找到第一个匹配元素就返回
auto it = std::find(ex::par, v.begin(), v.end(), target);
// 可能在 v[100] 匹配，但返回 v[200] 的迭代器
// 因为线程 A 处理 [0, 1000)，线程 B 处理 [1000, 2000)
// B 在 1000 找到，A 在 500 找到，但 B 先返回
```

## reduce 浮点陷阱

```cpp
std::vector<double> v = {1.0, 1e20, -1e20, 1.0};

// 串行 accumulate：(1.0 + 1e20) - 1e20 + 1.0 = 0.0 + 1.0 = 1.0
double s1 = std::accumulate(v.begin(), v.end(), 0.0);

// 并行 reduce：分组方式不同，可能 (1.0 + 1e20) + (-1e20 + 1.0) = 1e20 + 1.0 ≈ 1e20
double s2 = std::reduce(ex::par, v.begin(), v.end(), 0.0);
// s1 ≠ s2！浮点结合律不成立
```

## 自测题

1. C++17 给多少算法加了并行版本？举出三类。
2. `find(par)` 找到后是否立即停止？为什么？
3. `reduce(par)` 的浮点结果为什么可能与串行不同？结合律和浮点的关系？
4. `sort(par)` 的复杂度变了吗？比较次数呢？
5. `partition(par)` 和 `stable_partition(par)` 的区别？

<details>
<summary>参考答案</summary>

1. C++17 给 `<algorithm>` 与 `<numeric>` 中的**绝大多数**算法增加了接受执行策略的重载，数量在 60～70 个量级（确切清单与数目请以 cppreference 的并行算法列表为准）。
三类典型例子：**遍历与变换类**（`for_each`、`for_each_n`、`transform`、`copy`、`fill`、`generate`）、**查找与计数类**（`find`、`find_if`、`count`、`any_of`/`all_of`/`none_of`、`min_element`/`max_element`）、**数值与重排类**（`reduce`、`transform_reduce`、`exclusive_scan`/`inclusive_scan`、`sort`、`partition`）。
2. **不保证立即停止**：并行实现把区间切成多块并发处理，某块命中时其他线程可能已经在算后面的块，算法不会也无法立刻取消它们。
因此并行 `find` 不能当作串行的「第一个」来依赖——实现只保证「找到一个」，返回哪一个可以不同。若业务上必须取最靠前的那个，用 `seq`，或自己分块后取最小下标。
3. 并行 `reduce` 会把序列切成若干块、块内先归约再合并，加法的**分组顺序与串行左折叠不同**。而浮点加法**不满足结合律**：每步运算都要按有限精度舍入，`(a+b)+c` 与 `a+(b+c)` 的舍入点不同结果就不同；极端情况（如 `1e20 + 1.0`）还会出现大数吞小数。
所以结果与串行不同，而且分组方式可能随线程数变化，**不可复现**——回测这类要求可复现的场景要固定用 `seq` 或固定分块策略。
4. **复杂度量级不变**：带执行策略的 `sort` 仍要求 O(N log N) 次比较，标准对并行 `sort` 的比较复杂度要求与串行一致。
但**实际比较次数可能更多**：并行分区/归并会引入额外的比较与数据搬运。另外 `sort` 的比较器必须线程安全，且并行 `sort` 不保证稳定性（需要稳定用 `stable_sort`）。
5. `partition(par, ...)` **不保证**保持被划分元素的原有相对顺序，只保证谓词为真的元素都排在前半部分。
`stable_partition` 保持相对顺序，代价是可能更慢并且需要额外内存；C++17 中 `stable_partition` 也接受执行策略，但需要保序时就得接受这部分开销。

</details>
