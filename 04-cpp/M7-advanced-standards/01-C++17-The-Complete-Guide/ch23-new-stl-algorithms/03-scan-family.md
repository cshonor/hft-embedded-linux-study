# 前缀和家族

## exclusive_scan vs inclusive_scan

```cpp
std::vector<int> v = {1, 2, 3, 4};
std::vector<int> out(4);

// exclusive_scan：out[i] = v[0] + ... + v[i-1]（不含 v[i]）
std::exclusive_scan(v.begin(), v.end(), out.begin(), 0);
// out = [0, 1, 3, 6]

// inclusive_scan：out[i] = v[0] + ... + v[i]（含 v[i]）
std::inclusive_scan(v.begin(), v.end(), out.begin());
// out = [1, 3, 6, 10]
```

**区别**：
- `exclusive`：当前元素不参与自己的前缀和
- `inclusive`：当前元素参与自己的前缀和

## 带自定义操作

```cpp
// exclusive_scan 带自定义二元操作
std::vector<int> v = {2, 3, 4, 5};
std::vector<int> out(4);

// 前缀乘积（exclusive）
std::exclusive_scan(v.begin(), v.end(), out.begin(), 1, std::multiplies<>{});
// out = [1, 2, 6, 24]  → 1, 1*2, 1*2*3, 1*2*3*4
```

## transform_inclusive_scan / transform_exclusive_scan

```cpp
// 先 map 再前缀和
std::vector<int> v = {1, 2, 3, 4};
std::vector<int> out(4);

// transform_inclusive_scan：先对每个元素 map，再做 inclusive scan
std::transform_inclusive_scan(v.begin(), v.end(), out.begin(),
    std::plus<>{},          // scan 操作
    [](int x) { return x * 2; }  // map 操作
);
// map 后：[2, 4, 6, 8]
// scan 后：[2, 6, 12, 20]

// transform_exclusive_scan
std::transform_exclusive_scan(v.begin(), v.end(), out.begin(),
    0,                       // 初值
    std::plus<>{},           // scan 操作
    [](int x) { return x * 2; }  // map 操作
);
// map 后：[2, 4, 6, 8]
// scan 后：[0, 2, 6, 12]  → exclusive，不含当前
```

## 并行版本

```cpp
// 所有 scan 算法都支持执行策略
std::exclusive_scan(ex::par, v.begin(), v.end(), out.begin(), 0);
std::inclusive_scan(ex::par, v.begin(), v.end(), out.begin());
```

## 实际应用

```cpp
// 累计成交量
std::vector<int> trades = {100, 200, 150, 300};
std::vector<int> cumulative(trades.size());
std::inclusive_scan(trades.begin(), trades.end(), cumulative.begin());
// cumulative = [100, 300, 450, 750]

// 累计 PnL
std::vector<double> pnls = {10.5, -5.0, 20.0, -3.0};
std::vector<double> cumulative_pnl(pnls.size());
std::inclusive_scan(pnls.begin(), pnls.end(), cumulative_pnl.begin());
// cumulative_pnl = [10.5, 5.5, 25.5, 22.5]
```

## 自测题

1. `exclusive_scan` 和 `inclusive_scan` 的输出有什么区别？给出 `{1,2,3,4}` 的各自输出。
2. 初值在 `exclusive_scan` 和 `inclusive_scan` 中的角色分别是什么？
3. `transform_inclusive_scan` 的 map 和 scan 各做什么？
4. 所有 scan 算法支持执行策略吗？
5. 用 `inclusive_scan` 计算累计成交量的写法？

<details>
<summary>参考答案</summary>

1. 对 `{1, 2, 3, 4}`：
   - `exclusive_scan(v.begin(), v.end(), out.begin(), 0)` 得到 `out = [0, 1, 3, 6]`（`out[i]` 是 `v[0..i-1]` 的和，**不含** `v[i]`）。
   - `inclusive_scan(v.begin(), v.end(), out.begin())` 得到 `out = [1, 3, 6, 10]`（`out[i]` 是 `v[0..i]` 的和，**含** `v[i]`）。
两者输出长度都等于输入长度，默认运算为 `std::plus<>`。
2. `exclusive_scan` 的 `init` 是**必需的**：它就是 `out[0]`，是整个前缀和的起点，`out[i] = init ⊕ v[0] ⊕ ... ⊕ v[i-1]`。
`inclusive_scan` 有**不带** `init` 的重载（`out[0] = v[0]`），也有带 `init` 和 `binary_op` 的重载（`out[i] = init ⊕ v[0] ⊕ ... ⊕ v[i]`，此时 `init` 参与每一个元素）。
3. `transform_inclusive_scan(first, last, out, binary_op, unary_op)` 里：`unary_op` 是 **map**，先作用在每个输入元素上；`binary_op` 是 **scan**，再做前缀和归约。
例：输入 `{1,2,3,4}`、`unary_op` 为 `x*2`、`binary_op` 为 `plus`，则 map 后是 `{2,4,6,8}`，scan 后是 `{2,6,12,20}`。
注意参数顺序是**先二元（归约）后一元（map）**，容易写反。
4. 支持。`exclusive_scan`、`inclusive_scan`、`transform_exclusive_scan`、`transform_inclusive_scan` 都有接受执行策略的重载，可写成 `std::inclusive_scan(std::execution::par, v.begin(), v.end(), out.begin())` 等。
但前缀和本质上是**有依赖的递推**，并行实现需要多趟（分块求和 → 块间前缀和 → 回填），加速比不如 `reduce` 那么理想，是否划算要实测。
5. ```cpp
std::vector<int> trades = {100, 200, 150, 300};
std::vector<int> cumulative(trades.size());
std::inclusive_scan(trades.begin(), trades.end(), cumulative.begin());
// cumulative = [100, 300, 450, 750]
```
输出区间要先分配好空间；也可以用 `std::back_inserter(cumulative)` 代替裸迭代器以避免长度不匹配。

</details>
