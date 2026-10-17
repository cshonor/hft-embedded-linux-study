# 6.4 accumulate 与 for_each
> 第 6 章 算法 · 第 4 节 · 上一节：[6.3 copy 特化](03-copy-specialization.md) · 下一节：[第 7 章 仿函数](../ch07-functors/README.md)

## 为什么要学这个（先建立直觉）

C 里求和/遍历是手写循环，类型安全靠人工保证：

```c
// C: 手写求和，溢出和类型错误自己负责
int arr[] = {1, 2, 3, 4, 5};
int sum = 0;
for (int i = 0; i < 5; ++i) sum += arr[i];
// 如果 arr 是 long long，sum 是 int → 静默截断溢出
// 如果想求积？改循环逻辑
```

C++ 的 `std::accumulate` 类型安全、可自定义操作：

```cpp
auto sum = std::accumulate(v.begin(), v.end(), 0);    // int 求和
auto sum_ll = std::accumulate(v.begin(), v.end(), 0LL); // long long 求和
auto product = std::accumulate(v.begin(), v.end(), 1,
    std::multiplies<>{});  // 求积
```

`for_each` 则是类型安全的遍历——返回仿函数副本，可取最终状态。

## 这节讲什么

`accumulate` 和 `for_each` 是 STL 中最常用的"累积"和"遍历"算法，它们通过泛型接口适配任意操作。

### accumulate 源码

```cpp
// SGI accumulate 实现（简化）
template<class InputIterator, class T>
T accumulate(InputIterator first, InputIterator last, T init) {
    for (; first != last; ++first)
        init = init + *first;  // 默认用 +
    return init;
}

template<class InputIterator, class T, class BinaryOperation>
T accumulate(InputIterator first, InputIterator last, T init,
             BinaryOperation binary_op) {
    for (; first != last; ++first)
        init = binary_op(init, *first);  // 自定义二元操作
    return init;
}
```

关键：**初值类型 T 决定结果类型**——`init` 是 `int` 则结果 `int`，是 `long long` 则结果 `long long`。

### 初值类型陷阱

```cpp
std::vector<int> v = {1000000, 1000000, 1000000, 1000000, 1000000};

auto sum1 = std::accumulate(v.begin(), v.end(), 0);
// 初值 0 是 int → 结果 int → 溢出！5000000 超过 INT_MAX？不超，但更大的会

auto sum2 = std::accumulate(v.begin(), v.end(), 0LL);
// 初值 0LL 是 long long → 结果 long long → 安全

auto sum3 = std::accumulate(v.begin(), v.end(), 0.0);
// 初值 0.0 是 double → 结果 double → 浮点求和
```

### for_each 源码

```cpp
// SGI for_each 实现（简化）
template<class InputIterator, class Function>
Function for_each(InputIterator first, InputIterator last, Function f) {
    for (; first != last; ++first)
        f(*first);  // 对每个元素调用 f
    return f;  // 返回仿函数副本！
}
```

关键：`for_each` **返回仿函数副本**——可以用来收集遍历状态。

### for_each 返回值用法

```cpp
// 用 for_each 收集状态
struct Summarizer {
    int sum = 0;
    int count = 0;
    void operator()(int x) { sum += x; ++count; }
};

auto result = std::for_each(v.begin(), v.end(), Summarizer{});
std::cout << result.sum << " " << result.count;
// result 是 Summarizer 的副本，携带最终状态

// C++11 后 lambda 更简洁
int sum = 0;
std::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
// 但 lambda 无法"返回"——引用捕获是另一种方式
```

### accumulate 自定义操作

```cpp
// 求积
auto product = std::accumulate(v.begin(), v.end(), 1,
    [](int a, int b) { return a * b; });

// 拼接字符串
std::vector<std::string> words = {"hello", " ", "world"};
auto sentence = std::accumulate(words.begin(), words.end(), std::string{});

// 求最大值（初值用第一个元素或最小值）
auto max_val = std::accumulate(v.begin() + 1, v.end(), v[0],
    [](int a, int b) { return std::max(a, b); });
// 但 std::max_element 更直观
```

## 常见错误（新手踩坑）

### 错误 1：初值类型导致溢出

```cpp
// ❌ 初值 0 是 int，大数求和溢出
std::vector<long long> v = {3000000000LL, 3000000000LL};
auto sum = std::accumulate(v.begin(), v.end(), 0);
// 结果是 int → 溢出！截断为 -1294967296

// ✅ 初值用 0LL
auto sum = std::accumulate(v.begin(), v.end(), 0LL);  // long long，安全
```

### 错误 2：accumulate 连接字符串低效

```cpp
// ❌ 每次 += 可能触发 string 扩容
std::vector<std::string> words = {"a", "b", "c", "d", "e"};
auto result = std::accumulate(words.begin(), words.end(), std::string{});
// 每次调用 string::operator+= 可能 realloc

// ✅ 先算总长度再 reserve
size_t total = 0;
for (auto& w : words) total += w.size();
std::string result;
result.reserve(total);
std::accumulate(words.begin(), words.end(), std::ref(result));
// 或直接用循环
```

### 错误 3：for_each 中修改元素

```cpp
// ❌ for_each 接收的是元素值（拷贝），修改不影响原容器
std::vector<int> v = {1, 2, 3, 4, 5};
std::for_each(v.begin(), v.end(), [](int x) { x *= 2; });
// v 仍然是 {1, 2, 3, 4, 5}，x 是副本

// ✅ 用引用
std::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });
// v 变成 {2, 4, 6, 8, 10}
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写循环求和 | `std::accumulate` | C++ 泛型、可自定义操作 |
| 类型安全靠人工 | 初值类型决定结果类型 | C++ 编译期检查 |
| 手写遍历回调 | `std::for_each` + lambda | C++ 类型安全 |
| 无状态返回 | `for_each` 返回仿函数副本 | C++ 可收集状态 |

## HFT 关联

- **成交量求和**：`std::accumulate(volumes.begin(), volumes.end(), 0LL)` 用 `0LL` 避免 int 溢出——成交量可能很大
- **for_each 遍历订单簿**：`for_each(bids.begin(), bids.end(), process_order)` 类型安全，lambda 可内联
- **避免 accumulate 连接字符串**：热路径用预分配的 buffer + memcpy，不用 `string +=`
- **reduce 并行化**：C++17 `std::reduce` 可并行（但 HFT 单线程热路径用 accumulate 足够）

## 代码自测

### Q1: accumulate 的初值为什么决定结果类型？

```cpp
std::vector<int> v = {1, 2, 3};
auto a = std::accumulate(v.begin(), v.end(), 0);    // int
auto b = std::accumulate(v.begin(), v.end(), 0.0);  // double
auto c = std::accumulate(v.begin(), v.end(), 0LL);  // long long
```
> a、b、c 的类型分别是什么？为什么？

<details>
<summary>答案与复习指引</summary>

**类型由初值 T 决定**（模板推导）：
- `a`：T = `int`（初值 0 是 int），`init = init + *first` 都是 int → 结果 `int`
- `b`：T = `double`（初值 0.0），`init = init + *first` 是 double + int → double → 结果 `double`
- `c`：T = `long long`（初值 0LL），结果 `long long`

**陷阱**：
```cpp
std::vector<double> v = {1.5, 2.5, 3.5};
auto bad = std::accumulate(v.begin(), v.end(), 0);
// bad = 7（int！小数被截断）而不是 7.5
```

**HFT**：成交量求和一定用 `0LL`（long long），避免 int 溢出。

**复习：** → [初值类型陷阱](./04-accumulate-foreach.md)
</details>

### Q2: for_each 返回仿函数有什么用？

```cpp
struct Counter {
    int even = 0, odd = 0;
    void operator()(int x) {
        if (x % 2 == 0) ++even;
        else ++odd;
    }
};

std::vector<int> v = {1,2,3,4,5,6};
auto c = std::for_each(v.begin(), v.end(), Counter{});
std::cout << c.even << " " << c.odd;  // ?
```
> 输出什么？for_each 返回的是什么？

<details>
<summary>答案与复习指引</summary>

**输出 `3 3`**（3 个偶数 2,4,6，3 个奇数 1,3,5）。

**for_each 返回**：传入的仿函数的**副本**（按值返回）。这个副本携带着遍历过程中累积的状态。

**C++11 后**：lambda + 引用捕获更常见：
```cpp
int even = 0, odd = 0;
std::for_each(v.begin(), v.end(), [&](int x) {
    if (x % 2 == 0) ++even; else ++odd;
});
// even=3, odd=3
```

但 `for_each` 返回仿函数的方式在需要**可复用的有状态仿函数**时仍有价值。

**复习：** → [for_each 返回值用法](./04-accumulate-foreach.md)
</details>

### Q3: 下面的代码有什么问题？

```cpp
std::vector<int> v = {100000, 200000, 300000, 400000, 500000};
long long total = std::accumulate(v.begin(), v.end(), 0);
std::cout << total;  // 期望 1500000
```
> 结果正确吗？为什么？

<details>
<summary>答案与复习指引</summary>

**结果可能错误**（虽然 1500000 不超过 INT_MAX=2147483647，这里恰好正确，但更大的数据会溢出）。

**问题**：初值 `0` 是 `int`，所以 `accumulate` 内部用 `int` 累加。即使赋值给 `long long total`，累加过程中已经溢出了。

**修复**：
```cpp
long long total = std::accumulate(v.begin(), v.end(), 0LL);
// 初值 0LL → 内部用 long long 累加 → 不会溢出
```

**教训**：**初值类型决定中间计算类型**，不是目标变量类型。

**HFT**：成交量可能超过 INT_MAX（如 10 亿股 × 价格），务必用 `0LL`。

**复习：** → [初值类型陷阱](./04-accumulate-foreach.md)
</details>

### Q4: accumulate 和 reduce 有什么区别？

```cpp
// C++11: accumulate（不保证结合律，不能并行）
auto a = std::accumulate(v.begin(), v.end(), 0);

// C++17: reduce（要求结合律，可并行）
auto b = std::reduce(v.begin(), v.end(), 0);

// C++17 并行版
auto c = std::reduce(std::execution::par, v.begin(), v.end(), 0);
```
> 什么时候用 reduce？

<details>
<summary>答案与复习指引</summary>

**区别**：

| 特性 | `accumulate` | `reduce` (C++17) |
|------|------------|-----------------|
| 结合律 | 不要求 | 要求（可乱序求值） |
| 并行 | 不支持 | 支持 `execution::par` |
| 操作顺序 | 严格左结合 | 任意顺序 |
| 浮点 | 精确（顺序确定） | 可能不同（乱序） |
| 初值 | 必须提供 | 可省略（用默认构造） |

**什么时候用 reduce**：
- 数据量大（百万+）且操作满足结合律
- 需要并行加速（多核）
- 整数求和/求积（结合律满足）

**什么时候用 accumulate**：
- 浮点求和（reduce 乱序可能导致精度差异）
- 操作不满足结合律（如字符串连接、矩阵乘法）
- 数据量小（并行开销 > 收益）

**HFT**：热路径单线程用 accumulate（避免并行调度开销），回测大数据用 `reduce(par)`。

**复习：** → [accumulate 自定义操作](./04-accumulate-foreach.md)
</details>

## 参考与延伸

- 上一节：[6.3 copy 特化](03-copy-specialization.md)
- 下一节：[第 7 章 仿函数](../ch07-functors/README.md)
- 源码参考：`bits/stl_numeric.h`（`accumulate`）、`bits/stl_algo.h`（`for_each`）
