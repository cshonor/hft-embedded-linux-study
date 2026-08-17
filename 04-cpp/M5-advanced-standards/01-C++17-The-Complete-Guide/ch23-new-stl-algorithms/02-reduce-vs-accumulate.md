# reduce vs accumulate

## 核心区别

| 特性 | `std::accumulate` | `std::reduce` |
|------|-------------------|---------------|
| 执行方式 | 严格顺序（左折叠） | 可并行 |
| 初值类型 | 决定结果类型 | 独立于元素类型 |
| 操作要求 | 无特殊要求 | 须满足结合律 |
| 引入版本 | C++98 | C++17 |
| 头文件 | `<numeric>` | `<numeric>` |

## 初值类型陷阱

```cpp
std::vector<double> v = {1.5, 2.5, 3.5};

// accumulate：初值 0（int）→ 结果截断成 int！
int bad = std::accumulate(v.begin(), v.end(), 0);
// 0 + 1.5 = 1（截断）→ 1 + 2.5 = 3 → 3 + 3.5 = 6

// accumulate：初值 0.0（double）→ 正确
double good = std::accumulate(v.begin(), v.end(), 0.0);  // 7.5

// reduce：初值 0.0，类型独立
double also_good = std::reduce(v.begin(), v.end(), 0.0);  // 7.5
```

**陷阱**：`accumulate(v, 0)` 对 `vector<double>` 会把每次累加截断成 int。`reduce` 的初值参数类型独立，更不容易犯这个错。

## 结合律要求

```cpp
// accumulate：不要求结合律（严格左折叠）
// (0 + a) + b) + c) + d

// reduce：要求结合律（并行分组合并）
// 可能 (a + b) + (c + d)，也可能 ((a + b) + c) + d
// 如果操作不满足结合律，结果不确定

// 浮点加法不满足结合律
std::vector<double> v = {1e20, 1.0, -1e20};
double acc = std::accumulate(v.begin(), v.end(), 0.0);  // (1e20 + 1.0) - 1e20 = 0.0
double red = std::reduce(ex::par, v.begin(), v.end(), 0.0);
// 可能 (1e20 + 1.0) + (-1e20) = 0.0
// 也可能 1e20 + (1.0 + (-1e20)) = 1e20 + 0.0 = 1e20
// 结果不确定！
```

## 性能对比

```cpp
// 大数据集：reduce(par) 通常更快
std::vector<long> big(10'000'000);
// ... fill ...

auto t1 = now();
long s1 = std::accumulate(big.begin(), big.end(), 0L);  // 串行
auto t2 = now();
long s2 = std::reduce(ex::par, big.begin(), big.end(), 0L);  // 并行
auto t3 = now();

// reduce(par) 通常快 2-4 倍（取决于核数和数据量）
```

## 自测题

1. `accumulate` 的初值类型如何影响结果？举例说明截断陷阱。
2. `reduce` 为什么要求操作满足结合律？浮点加法满足吗？
3. `accumulate` 和 `reduce` 的执行方式分别是什么？
4. 大数据集上 `reduce(par)` 比 `accumulate` 快多少？小数据呢？
5. 如果操作不满足结合律，用 `reduce` 会怎样？
