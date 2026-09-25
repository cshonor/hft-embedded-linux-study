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

<details>
<summary>参考答案</summary>

1. `accumulate` 的返回类型就是**初值的类型**，中间每一步都先换算到该类型再累加。
经典陷阱：`std::vector<double> v{0.5, 0.5, 0.5}; std::accumulate(v.begin(), v.end(), 0)` 的初值是 `int`，每一步 `0 + 0.5` 的结果被截断回 `int`，最终得到 `0` 而不是 `1.5`。
正确写法是 `std::accumulate(v.begin(), v.end(), 0.0)`（初值用 `double`），或显式指定初值类型。
2. `reduce` 允许把区间切成任意块、以任意顺序合并，只有运算满足**结合律**（`(a∘b)∘c == a∘(b∘c)`）时结果才与分组方式无关。
浮点加法**不满足**结合律：每步都按有限精度舍入，`(1e20 + 1.0) - 1e20 = 0.0`，而 `1e20 + (1.0 - 1e20) = 1e20`，换一种分组结果就差了 20 个数量级。
3. `accumulate` 是**严格左折叠**：`init`、`init+a`、`(init+a)+b`…，固定顺序执行，不要求结合律或交换律，因此逐元素结果可复现。
`reduce` 允许**任意分组与顺序**（并行时分块归约再合并），标准只要求二元运算满足结合律（不要求交换律），顺序不确定，所以结果可能与 `accumulate` 不同。
4. 数量级上，大数据集（百万级以上、每元素操作很轻）时 `reduce(par)` 常常能拿到数倍加速，但具体倍数取决于核数、内存带宽与实现（MSVC/PPL、libstdc++ + TBB 等差异很大），**必须实测**，不要写死数字。
小数据集则相反：任务划分、线程调度与同步的固定开销超过计算本身，`reduce(par)` 通常比 `accumulate` 更慢。
5. 结果会**依赖分块方式和线程数**，变成不确定、不可复现的值（例如「减」这类非结合运算，不同分组会给出不同答案）。
标准并未把它判为未定义行为，只是不再保证唯一结果——所以把非结合运算塞进 `reduce(par)` 是逻辑错误而不是编译错误，排查起来更隐蔽。

</details>
