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

<details>
<summary>参考答案</summary>

1. **一次遍历完成 map + reduce**，省掉了 `transform` 写中间结果的那一次完整遍历和中间存储，对 cache 与内存带宽更友好，代码也更短。
此外 `transform_reduce` 支持执行策略，map 阶段天然可并行；而 `transform` + `reduce` 两段各自处理，多一次数据落地。
2. 两种重载语义不同：
   1. **内积形式**（两个区间）：`transform_reduce(first1, last1, first2, init)`，等价于 `init + (a0*b0) + (a1*b1) + ...`，二元运算固定为加法与乘法。
   2. **map + reduce 形式**（单区间）：`transform_reduce(first, last, init, binary_op, unary_op)`，先对每个元素做 `unary_op`（map），再用 `binary_op` 归约，初值为 `init`。
第二种更通用：把 `unary_op` 设为恒等、`binary_op` 设为 `std::plus<>` 就退化成 `reduce`。
3. **结合律**：并行把区间分块后任意合并，只有 `(a∘b)∘c == a∘(b∘c)` 时结果才与分组方式无关；否则结果依赖线程数和分块方式，变成不确定、不可复现的值。
**map 不能有副作用**：`unary_op` 会被并发调用，调用次数与顺序标准未作规定，实现还可能复制函数对象；一旦有写共享状态之类的副作用就会产生数据竞争（未定义行为）或不确定结果。同理两个 op 都不应使迭代器失效、不应修改区间元素。
4. 用内积形式的重载最直接：
```cpp
double dot = std::transform_reduce(
    std::execution::par,
    a.begin(), a.end(), b.begin(),
    0.0);          // init 必须是对 + 的幺元
```
它等价于 `0.0 + a[0]*b[0] + a[1]*b[1] + ...`。若要显式指定运算，可用七参数版本 `transform_reduce(par, a.begin(), a.end(), b.begin(), 0.0, std::plus<>{}, std::multiplies<>{})`。
5. 并行 `reduce` 把区间切成 k 块，每块都以 `init` 为起点归约，最后再把 k 个结果合并——`init` 实际参与了 **k 次**运算，而串行只参与 1 次。
只有当 `init` 是 `binary_op` 的**幺元**（identity：`std::plus` 为 `0`，`std::multiplies` 为 `1`，`std::min` 为极大值）时，多出来的那些 `init` 才不改变结果，并行与串行的答案才一致。

</details>
