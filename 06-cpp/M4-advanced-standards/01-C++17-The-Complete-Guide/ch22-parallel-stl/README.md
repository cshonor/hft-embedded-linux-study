# 第 22 章 并行 STL 算法

**Parallel STL Algorithms**

## 本章讲什么

C++17 给 `<algorithm>` 加了执行策略参数，让算法可并行/向量化运行。本章详述 `seq`/`par`/`par_unseq` 三种策略、并行算法的语义约束、以及性能预期。（概念基础见 [08 第 10 章](../../../M2-deep-principles/02-Cpp-Concurrency/ch10-parallel-algorithms/)，本章是 C++17 视角的深入。）

## 要点

### 三种执行策略（回顾）

```cpp
#include <execution>
namespace ex = std::execution;

std::sort(ex::seq, v.begin(), v.end());        // 顺序
std::sort(ex::par, v.begin(), v.end());        // 并行
std::for_each(ex::par_unseq, v.begin(), v.end(), f);  // 并行+向量化
```

### 并行化的算法清单

C++17 给**几乎所有** `<algorithm>` 算法加了并行版本：
- `for_each`、`transform`、`reduce`、`fill`、`generate`
- `copy`、`move`、`swap_ranges`
- `sort`、`stable_sort`、`partial_sort`、`nth_element`
- `find`、`find_if`、`count`、`any_of`/`all_of`/`none_of`
- `partition`、`merge`、`unique`
- `min_element`/`max_element`/`minmax_element`

### 语义约束差异

| 算法类别 | 并行约束 |
|----------|----------|
| `for_each` | 不可修改序列长度，函数对象不保证调用顺序 |
| `reduce` | 操作须满足**结合律**（不要求交换律）→ 浮点结果可能不同 |
| `sort(par)` | 复杂度仍 O(n log n)，但比较次数可能更多 |
| `fill`/`copy` | 并行写入不重叠区域，安全 |
| `find(par)` | 找到后**不保证立即停止**（其他线程可能已在处理） |

### `transform_reduce`：map+reduce

```cpp
// 计算 sum of squares
double result = std::transform_reduce(
    ex::par,
    v.begin(), v.end(),
    0.0,                       // 初值
    std::plus<double>{},       // reduce 操作
    [](double x) { return x * x; }  // map 操作
);
```

`transform_reduce` 是 `transform` + `reduce` 的融合，减少一次遍历，性能更好。

### `exclusive_scan`/`inclusive_scan`

C++17 新增的前缀和算法，可并行化：

```cpp
// 前缀和：[1,2,3,4] → [0,1,3,6]（exclusive）
std::vector<int> v = {1,2,3,4}, out(4);
std::exclusive_scan(ex::par, v.begin(), v.end(), out.begin(), 0, std::plus<>{});
// out = [0, 1, 3, 6]
```

### 性能预期

- **大数据 + 简单操作**：并行收益大（如百万级 `for_each`）。
- **小数据**：并行启动开销 > 收益，反而更慢。
- **复杂操作**：操作本身耗时占比大，并行收益相对小。
- **实现差异**：MSVC（PPL）、libstdc++（可选 TBB）、icc（TBB）性能差异大，需实测。

## HFT 关联

- **回测批处理**：百万级历史 tick 的统计（均值、方差、分位数）用 `transform_reduce(par)` 加速。
- **`reduce` 浮点陷阱**：并行 reduce 改变加法结合顺序，浮点结果与串行不同——回测复现要固定策略（用 `seq` 或固定分块）。
- **`exclusive_scan` 做累计统计**：累计成交量、累计 PnL 用前缀和并行计算。
- **热路径不用并行 STL**：单笔行情数据量小（1-10 个字段），启动开销 > 收益。
- **C++17 并行算法的实现对 HFT 不透明**：不知道内部线程池/调度，确定性延迟不保证——HFT 热路径仍用手写绑核流水线。
- **离线分析适用**：盘后统计分析、日志聚合用 `for_each(par)` 加速。

## 自测题

1. 三种执行策略的并行/向量化能力分别是什么？
2. 为什么 `reduce(par)` 的浮点结果可能与串行不同？
3. `transform_reduce` 相比 `transform` + `reduce` 有什么优势？
4. `find(par)` 找到后是否立即停止？为什么？
5. HFT 热路径为什么不用并行 STL？什么场景适用？
