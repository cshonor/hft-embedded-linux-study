# std::gcd / std::lcm

## 基本用法

```cpp
#include <numeric>

// gcd：最大公约数
int g = std::gcd(12, 18);   // 6
int g2 = std::gcd(7, 13);   // 1（互质）

// lcm：最小公倍数
int l = std::lcm(4, 6);     // 12
int l2 = std::lcm(3, 5);    // 15
```

## constexpr 支持

```cpp
// C++17：gcd/lcm 是 constexpr
constexpr int g = std::gcd(12, 18);   // 编译期计算
constexpr int l = std::lcm(4, 6);     // 编译期计算

// 可用于模板参数
template <int N, int M>
struct Period {
    static constexpr int common = std::gcd(N, M);
};
```

## 数学性质

```cpp
// gcd(0, n) = n
static_assert(std::gcd(0, 5) == 5);

// gcd(n, 0) = n
static_assert(std::gcd(5, 0) == 5);

// gcd(a, b) * lcm(a, b) = |a * b|（当 a, b != 0）
static_assert(std::gcd(4, 6) * std::lcm(4, 6) == 4 * 6);  // 2 * 12 = 24

// 负数：gcd 返回非负
static_assert(std::gcd(-4, 6) == 2);
```

## 类型要求

```cpp
// 参数类型必须是有符号或无符号整数（不支持浮点）
std::gcd(4, 6);        // int → int
std::gcd(4L, 6L);      // long → long
std::gcd(4u, 6u);      // unsigned → unsigned
// std::gcd(4.0, 6.0); // ❌ 编译错误：不支持浮点

// 混合类型：返回公共类型
auto r = std::gcd(4, 6L);  // long
```

## 实际应用

```cpp
// 1. 多策略信号周期对齐
// 策略A 每 5 秒出信号，策略B 每 3 秒出信号
// 公共周期 = gcd(5, 3) = 1 秒
constexpr int common_period = std::gcd(5, 3);

// 2. 内存对齐计算
// 两个结构体大小 12 和 8 字节，求能同时容纳两者的最小块
constexpr int block = std::lcm(12, 8);  // 24

// 3. 分数化简
struct Fraction {
    int num, den;
    void simplify() {
        int g = std::gcd(num, den);
        num /= g;
        den /= g;
    }
};
```

## 自测题

1. `gcd(12, 18)` 和 `lcm(4, 6)` 的结果分别是什么？
2. `gcd` 和 `lcm` 是 `constexpr` 吗？能在编译期用吗？
3. `gcd` 支持浮点参数吗？混合整数类型怎么处理返回类型？
4. `gcd(0, n)` 等于什么？`gcd(-4, 6)` 呢？
5. 多策略信号周期 5 秒和 3 秒，公共采样周期怎么算？

<details>
<summary>参考答案</summary>

1. `std::gcd(12, 18)` 为 **6**（12 与 18 的最大公约数）；`std::lcm(4, 6)` 为 **12**（4 与 6 的最小公倍数）。
二者满足 `gcd(m,n) * lcm(m,n) == |m * n|`（在结果可表示的前提下），如 `gcd(4,6) * lcm(4,6) == 2 * 12 == 24`。
2. 是。C++17 起 `std::gcd` / `std::lcm` 都是 **`constexpr`**，可以在编译期求值：
```cpp
constexpr int g = std::gcd(12, 18);   // 编译期得到 6
static_assert(std::lcm(4, 6) == 12);
```
它们定义在 `<numeric>` 中，常用于编译期计算对齐、分块大小一类的常量。
3. **不支持浮点**：两个参数都必须是整数类型（有符号或无符号整型），`std::gcd(4.0, 6.0)` 直接编译不过。
混合整数类型时返回它们的**公共类型** `std::common_type_t<M, N>`，例如 `std::gcd(4, 6L)` 返回 `long`。
另外结果恒为**非负**；`lcm` 的结果若无法用返回类型表示，行为未定义。
4. `std::gcd(0, n)` 返回 `|n|`（`n` 的绝对值，非负），例如 `gcd(0, 5) == 5`；`std::gcd(0, 0)` 返回 `0`。
`std::gcd(-4, 6)` 返回 **2**——`gcd` 的结果恒为**非负**，符号被丢弃；负参数参与时 `lcm` 的结果同样取非负。
5. 要看「公共」指的是哪一种口径：
   - 若指**能同时命中两个策略信号时刻的采样间隔**（采样粒度），用 `gcd(5, 3) == 1`，即每 1 秒采样一次，5 秒和 3 秒的信号都不会漏（笔记里 `common_period` 取的就是这种口径）。
   - 若指**两个周期再次完全重合的重复周期**，用 `lcm(5, 3) == 15`，即每 15 秒两个策略的相位才回到初始关系。
两者都在 `<numeric>` 中且是 `constexpr`，可直接写成编译期常量。

</details>
