# 第 23 章 新 STL 算法详解

**New STL Algorithms in Detail**

## 本章讲什么

C++17 给 `<algorithm>` 加了几个新算法：`clamp`、`reduce`、`transform_reduce`、`exclusive_scan`/`inclusive_scan`/`transform_scan`、`gcd`/`lcm`。

## 要点

### `clamp`：限制范围

```cpp
#include <algorithm>

int x = std::clamp(val, lo, hi);   // val < lo 返回 lo，val > hi 返回 hi，否则 val
// 等价 std::max(lo, std::min(val, hi))

std::clamp(val, 0.0, 1.0, [](double a, double b){ return a < b; });  // 可带比较器
```

### `reduce`：比 `accumulate` 更友好并行

```cpp
// accumulate：串行，初值类型决定结果类型
int sum1 = std::accumulate(v.begin(), v.end(), 0);   // int 累加，double 截断！

// reduce：可并行，初值类型与元素类型可不同
double sum2 = std::reduce(v.begin(), v.end(), 0.0);  // double 累加，正确

// reduce 要求结合律，accumulate 不要求
// reduce 可并行，accumulate 不行
```

`accumulate` 的陷阱：初值类型决定累加类型，`accumulate(v, 0)` 对 `vector<double>` 会截断成 int。`reduce` 的初值参数类型独立，更安全。

### `transform_reduce`：map + reduce 融合

```cpp
// 内积：sum of (a[i] * b[i])
double dot = std::transform_reduce(
    a.begin(), a.end(), b.begin(), 0.0
);

// 自定义 map + reduce
double result = std::transform_reduce(
    v.begin(), v.end(),
    0.0,
    std::plus<>{},
    [](double x) { return x * 2; }
);
```

### 前缀和家族

```cpp
std::vector<int> v = {1, 2, 3, 4}, out(4);

// exclusive_scan：out[i] = v[0] + ... + v[i-1]（不含 v[i]）
std::exclusive_scan(v.begin(), v.end(), out.begin(), 0);
// out = [0, 1, 3, 6]

// inclusive_scan：out[i] = v[0] + ... + v[i]（含 v[i]）
std::inclusive_scan(v.begin(), v.end(), out.begin());
// out = [1, 3, 6, 10]

// transform_scan：先 map 再前缀和
std::transform_inclusive_scan(v.begin(), v.end(), out.begin(),
    std::plus<>{}, [](int x){ return x*2; });
// out = [2, 6, 12, 20]
```

### `gcd` / `lcm`

```cpp
#include <numeric>

int g = std::gcd(12, 18);   // 6
int l = std::lcm(4, 6);     // 12

// C++17 constexpr
constexpr int g2 = std::gcd(12, 18);  // 编译期
```

## HFT 关联

- **`clamp` 限制价格滑点**：`auto fill_px = clamp(order_px, best_bid, best_ask)` 防止价格超出最优报价。
- **`transform_reduce` 算 PnL**：`transform_reduce(trades, 0.0, plus{}, [](auto& t){ return t.qty * (t.sell_px - t.buy_px); })` 一行算总盈亏。
- **`reduce` 替代 `accumulate`**：避免初值类型截断陷阱，热路径统计用 `reduce` 更安全。
- **`gcd` 做周期对齐**：多策略信号周期不同，用 `gcd` 算公共周期对齐采样点。
- **`inclusive_scan` 累计统计**：累计成交量、累计 PnL 用前缀和，O(n) 一次算出所有时间点的累计值。
- **`clamp` 的比较器**：自定义价格比较逻辑（如考虑最小变动价位）用带比较器的 clamp。

## 自测题

1. `reduce` 相比 `accumulate` 有什么优势？为什么更安全？
2. `transform_reduce` 的 map 和 reduce 分别是什么？内积怎么算？
3. `exclusive_scan` 和 `inclusive_scan` 的区别？输出示例？
4. `clamp(val, lo, hi)` 等价于什么？能带比较器吗？
5. HFT 用 `transform_reduce` 算 PnL 的写法是什么？
