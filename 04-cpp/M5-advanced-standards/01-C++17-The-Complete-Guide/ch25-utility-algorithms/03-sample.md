# std::sample

## 基本用法

```cpp
#include <algorithm>
#include <random>

std::vector<int> pop = {1,2,3,4,5,6,7,8,9,10};
std::vector<int> result(3);

std::mt19937 rng{std::random_device{}()};
std::sample(pop.begin(), pop.end(),
            result.begin(), 3, rng);
// result 中有 3 个随机不重复元素
```

## 算法：蓄水池抽样

`std::sample` 使用 **蓄水池抽样（Reservoir Sampling）** 算法：

```
对于 n 个元素中取 k 个：
1. 前 k 个元素直接放入结果
2. 第 i 个元素（i > k）：以 k/i 的概率替换结果中的随机一个
3. 最终结果中每个元素被选中的概率都是 k/n
```

**复杂度**：O(n) 时间，O(k) 空间。
**保证**：均匀采样，每个子集被选中的概率相等。

## 迭代器要求

```cpp
// 输入迭代器：可以用于流式数据（不知道总大小）
std::istringstream iss("1 2 3 4 5 6 7 8 9 10");
std::vector<int> result(3);
std::sample(std::istream_iterator<int>(iss),
            std::istream_iterator<int>(),
            result.begin(), 3, rng);
// 从流中随机采样 3 个

// 输出迭代器：必须至少 ForwardIterator
// 如果 k > distance(first, last)，只采样可用元素
```

## 应用场景

```cpp
// 1. 从历史 tick 中随机抽样做快速回测
std::vector<Tick> all_ticks = load_ticks("2024-01-01");
std::vector<Tick> sample_ticks(10000);
std::sample(all_ticks.begin(), all_ticks.end(),
            sample_ticks.begin(), 10000, rng);
// 10000 个随机 tick 做快速验证

// 2. 日志采样
std::vector<LogEntry> logs = read_logs();
std::vector<LogEntry> sample_logs(100);
std::sample(logs.begin(), logs.end(),
            sample_logs.begin(), 100, rng);

// 3. 随机测试数据生成
std::vector<int> universe(1000);
std::iota(universe.begin(), universe.end(), 0);
std::vector<int> test_data(50);
std::sample(universe.begin(), universe.end(),
            test_data.begin(), 50, rng);
```

## 与 shuffle 的区别

```cpp
// shuffle：打乱全部，取前 k 个 → O(n) 但要修改原序列
std::shuffle(v.begin(), v.end(), rng);
auto sample = std::vector(v.begin(), v.begin() + k);

// sample：不修改原序列，O(n) 时间，适合只读数据
std::sample(v.begin(), v.end(), out.begin(), k, rng);
```

## 自测题

1. `std::sample` 用的什么算法？复杂度？
2. 蓄水池抽样的原理是什么？如何保证均匀？
3. `sample` 需要什么类型的迭代器？能用于流式数据吗？
4. `sample` 和 `shuffle` + 取前 k 个有什么区别？
5. HFT 中从历史 tick 抽样做快速回测的写法？

<details>
<summary>参考答案</summary>

1. 复杂度与 `std::distance(first, last)` 成**线性**，即 O(n)，并且不随 k 增长。
具体算法标准没有规定：实现可以采用 **selection sampling**（选择抽样，如 Floyd/Knuth 的算法，适用于区间长度已知的 ForwardIterator 及以上）或 **reservoir sampling**（蓄水池抽样，适用于只能单趟扫描的 InputIterator）。
`n` 大于区间元素个数时，只输出全部可用元素。
2. 蓄水池抽样（Algorithm R）：先把前 k 个元素放进「蓄水池」，之后对第 i 个元素（i > k，从 1 开始计）以概率 `k/i` 随机替换掉蓄水池中的某一个（每个位置被替换的概率为 `1/k`）。
均匀性由归纳法保证：处理到第 i 个元素时，池中任一元素留下来的概率都是 `k/i`，于是任一元素最终入选的概率都是 `k/n`。它只需要**单趟扫描**、不需要预先知道 n，所以能用于流式/输入迭代器。
3. 总体区间（`PopulationIterator`）至少需要 **InputIterator**；输出迭代器 `out` 至少是 **OutputIterator**，且当总体迭代器不满足 ForwardIterator 时，`out` 还必须是 **RandomAccessIterator**（因为蓄水池抽样需要回写/随机写）。
`out` 不能落在 `[first, last)` 区间内（否则未定义行为）。
所以**可以用于流式数据**——例如 `std::istream_iterator<int>` 这种 InputIterator，配一个随机访问输出迭代器（如已分配大小的 `vector::begin()`）即可单趟抽样。
4. `sample` **不修改原序列**，只往输出迭代器写，时间复杂度 O(n)，额外空间 O(k)；`shuffle` + 取前 k 个会**就地打乱整个序列**（O(n) 时间、O(1) 额外空间，但原顺序丢失），适合本来就允许被打乱的数据。
另外两者语义不同：`sample` 只在总体迭代器满足 ForwardIterator 时才**保持选中元素的相对顺序**（stable）；`shuffle` 后的顺序本来就是随机的。
5. ```cpp
std::vector<Tick> all_ticks = load_ticks("2024-01-01");
std::vector<Tick> sample_ticks(10000);      // 输出需预先分配
std::mt19937 rng{std::random_device{}()};

std::sample(all_ticks.begin(), all_ticks.end(),
            sample_ticks.begin(), 10000, rng);
```
若不想预分配，可用 `std::back_inserter(sample_ticks)`（此时总体是随机访问迭代器，输出只需 OutputIterator 即可）。
注意这是**无放回**抽样，且只保证「每个大小为 k 的子集等概率」，不保证样本在时间序列上均匀——回测结论要留意抽样偏差。

</details>
