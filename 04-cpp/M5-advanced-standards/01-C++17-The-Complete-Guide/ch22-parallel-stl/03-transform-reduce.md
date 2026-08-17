# transform_reduce 详解

## 基本形式

```cpp
#include <numeric>
#include <execution>

// 形式1：内积（两个序列）
double dot = std::transform_reduce(
    ex::par,
    a.begin(), a.end(),   // 第一序列
    b.begin(),             // 第二序列起始
    0.0                    // 初值
);
// 计算 sum(a[i] * b[i])

// 形式2：自定义 map + reduce
double result = std::transform_reduce(
    ex::par,
    v.begin(), v.end(),
    0.0,                   // reduce 初值
    std::plus<double>{},   // reduce 操作
    [](double x) { return x * x; }  // map 操作
);
// 计算 sum(v[i]^2)
```

## map + reduce 的融合

```
传统方式：先 transform 再 reduce
  tmp[i] = map(v[i])       → 一次遍历 + 临时存储
  result = reduce(tmp)      → 一次遍历

transform_reduce：融合
  result = reduce(map(v[i])) → 一次遍历，无临时存储
```

**优势**：
- 减少一次遍历
- 无临时容器分配
- 对 cache 友好（map 后立即 reduce）

## 实际应用

```cpp
// 1. 计算加权平均
std::vector<double> prices = {100.0, 101.0, 102.0};
std::vector<double> weights = {0.3, 0.5, 0.2};

double weighted_avg = std::transform_reduce(
    prices.begin(), prices.end(), weights.begin(), 0.0
) / std::accumulate(weights.begin(), weights.end(), 0.0);

// 2. 计算 PnL（盈亏）
struct Trade { int qty; double buy_px, sell_px; };
std::vector<Trade> trades;

double total_pnl = std::transform_reduce(
    ex::par,
    trades.begin(), trades.end(),
    0.0,
    std::plus<>{},
    [](const Trade& t) { return t.qty * (t.sell_px - t.buy_px); }
);

// 3. 向量范数
double norm = std::sqrt(std::transform_reduce(
    ex::par,
    v.begin(), v.end(),
    0.0,
    std::plus<>{},
    [](double x) { return x * x; }
));
```

## 与 reduce 的关系

```cpp
// reduce 是 transform_reduce 的特例（map = identity）
std::reduce(ex::par, v.begin(), v.end(), 0.0);

// 等价于
std::transform_reduce(ex::par, v.begin(), v.end(), 0.0,
                      std::plus<>{}, [](double x) { return x; });
```

## 语义约束

- **reduce 操作须满足结合律**：因为并行分组后合并，顺序不确定
- **map 操作须无副作用**：多线程可能同时对不同元素调用 map
- **初值是幺元**：对于 `plus`，初值 `0.0` 是幺元（`0 + x = x`）

## 自测题

1. `transform_reduce` 相比 `transform` + `reduce` 有什么优势？
2. 两种重载形式的区别？（内积形式 vs 自定义 map+reduce 形式）
3. reduce 操作为什么要满足结合律？map 操作为什么不能有副作用？
4. 用 `transform_reduce` 计算向量内积的写法？
5. 初值为什么必须是幺元？
