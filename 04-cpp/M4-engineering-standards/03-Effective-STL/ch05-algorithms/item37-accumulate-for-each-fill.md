# Item 37：`accumulate` / `for_each` / `fill` 的语义

> 第 5 章 算法 · Item 37 · 上一节：[Item 36 正确选择算法](item36-choose-right-algorithm.md)

## 为什么要学这个（先建立直觉）

C 程序员求和/填充用手写循环：

```c
// 求和
int sum = 0;
for (int i = 0; i < n; ++i) sum += arr[i];

// 填充
for (int i = 0; i < n; ++i) arr[i] = 0;
```

C++ 的 STL 提供了命名算法：

```cpp
int sum = std::accumulate(v.begin(), v.end(), 0);  // 求和
std::fill(v.begin(), v.end(), 0);                   // 填充
std::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });  // 遍历修改
```

---

## 这节讲什么

`accumulate` 默认求和，可挂自定义二元操作。`for_each` 遍历执行可变操作。`fill` 填充。`accumulate` 的初值类型决定结果类型——这是经典陷阱。

---

## 三个算法的语义

```cpp
// accumulate：累加（默认 +，可自定义）
int sum = std::accumulate(v.begin(), v.end(), 0);       // 0 + v[0] + v[1] + ...
int product = std::accumulate(v.begin(), v.end(), 1,
    std::multiplies<int>());  // 1 * v[0] * v[1] * ...
std::string concat = std::accumulate(v.begin(), v.end(), std::string(""),
    [](const std::string& a, int b) { return a + std::to_string(b); });  // 拼接

// for_each：遍历执行（可修改元素）
std::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });
// for_each 返回传入的函数对象（可获取最终状态）
struct Counter { int n = 0; void operator()(int) { ++n; } };
Counter c = std::for_each(v.begin(), v.end(), Counter{});
std::cout << c.n;  // v.size()

// fill：填充
std::fill(v.begin(), v.end(), 0);
std::fill_n(v.begin(), 5, 0);  // 只填前 5 个
```

### accumulate 初值类型陷阱

```cpp
std::vector<double> d = {1.5, 2.5, 3.5};

// ❌ 初始值 0 (int) → 所有加法用 int → 截断小数！
auto r1 = std::accumulate(d.begin(), d.end(), 0);  // r1 = 6 (int)

// ✅ 初始值 0.0 (double) → 加法用 double
auto r2 = std::accumulate(d.begin(), d.end(), 0.0);  // r2 = 7.5 (double)

// HFT：累加成交额用 0LL (long long)
std::vector<long long> volumes = {1000000, 2000000, 3000000};
auto total = std::accumulate(volumes.begin(), volumes.end(), 0LL);
// 0LL 确保用 long long 运算，避免 int 溢出
```

---

## 常见错误（新手踩坑）

### 错误 1：accumulate 初值类型错误

```cpp
std::vector<int> v = {100000, 200000, 300000};
auto sum = std::accumulate(v.begin(), v.end(), 0);  // 0 是 int → 可能溢出！
// 如果总和 > INT_MAX → UB
```

**修正：** 用 `0LL`（long long）：`std::accumulate(v.begin(), v.end(), 0LL);`

### 错误 2：for_each 以为会返回 void

```cpp
// for_each 返回传入的函数对象（副本）
struct Sum {
    int total = 0;
    void operator()(int x) { total += x; }
};
auto result = std::for_each(v.begin(), v.end(), Sum{});
std::cout << result.total;  // ✅ 获取累加结果
```

### 错误 3：fill_n 越界

```cpp
std::vector<int> v(5);
std::fill_n(v.begin(), 10, 0);  // ❌ 填充 10 个但 v 只有 5 个 → UB
```

**修正：** 确保数量不超过容器大小，或用 `std::fill(v.begin(), v.end(), 0);`

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 求和 | 手写循环 | `accumulate` | 命名表达意图 |
| 填充 | `memset` / 循环 | `fill` | 类型安全 |
| 遍历修改 | 循环 | `for_each` | 可返回状态 |
| 类型陷阱 | 无 | accumulate 初值类型 | 注意匹配 |

**一句话：** C 用手写循环求和/填充。C++ 的 `accumulate`/`fill`/`for_each` 用命名表达意图——但 `accumulate` 的初值类型决定结果类型，HFT 累加金额用 `0LL` 避免 int 溢出。

---

## HFT 关联

- **`accumulate` 初值类型陷阱**：累加成交额用 `0LL`（int64）而非 `0`（int），避免大额溢出——和定点价格铁律一致。
- **`for_each` 累积 PnL**：回测里用有状态仿函数累积 PnL，`for_each` 返回最终副本取结果。
- **`fill` 初始化缓冲**：预分配的 tick 缓冲用 `fill(0)` 初始化，比循环更清晰。

---

## 代码自测

### Q1: 初值类型
```cpp
std::vector<double> d = {1.5, 2.5, 3.5};
auto r1 = std::accumulate(d.begin(), d.end(), 0);    // A
auto r2 = std::accumulate(d.begin(), d.end(), 0.0);  // B
std::cout << r1 << ' ' << r2;
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `6 7.5`。
- **A**：初始值 `0` 是 `int` → 所有加法用 `int` 运算 → 1.5→1, 2.5→2, 3.5→3 → 0+1+2+3=6。
- **B**：初始值 `0.0` 是 `double` → 加法用 `double` → 0.0+1.5+2.5+3.5=7.5。

**教训**：`accumulate` 的返回类型由初始值决定。累加浮点数用 `0.0`，累加大整数用 `0LL`。
</details>

### Q2: 自定义操作
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto result = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());
std::cout << result;
```

<details>
<summary>答案</summary>

输出 `120`。`accumulate` 用 `multiplies<int>()` 代替默认的 `+`，计算 1×1×2×3×4×5 = 120。初始值 1 是乘法单位元。
</details>

### Q3: for_each 返回值
```cpp
struct Counter {
    int count = 0;
    void operator()(int x) { if (x > 5) ++count; }
};
std::vector<int> v = {3, 7, 2, 8, 1, 9};
Counter c = std::for_each(v.begin(), v.end(), Counter{});
std::cout << c.count;
```

<detailf>
<summary>答案</summary>

输出 `3`。`for_each` 返回传入的函数对象（副本），`c.count` = 3（7, 8, 9 三个 > 5 的元素）。

`for_each` 的返回值让有状态仿函数能获取最终状态——但注意是值拷贝，大状态用引用捕获。
</details>

### Q4: HFT 溢出
```cpp
std::vector<int> volumes = {1000000000, 1000000000, 1000000000};
// A
auto total_a = std::accumulate(volumes.begin(), volumes.end(), 0);
// B
auto total_b = std::accumulate(volumes.begin(), volumes.end(), 0LL);
std::cout << total_a << ' ' << total_b;
```

<detailf>
<summary>答案</summary>

- **A**：`0` 是 `int` → `0 + 10亿 + 10亿 + 10亿 = 30亿` → 超过 `INT_MAX`（约 21.5 亿）→ **有符号整数溢出 = UB**。实际输出可能是负数。
- **B**：`0LL` 是 `long long` → 30亿 在 `long long` 范围内 → 正确输出 3000000000。

**HFT 铁律**：累加金额/成交量用 `0LL`（int64），不用 `0`（int32）。大额累加用 int 会溢出。
</details>

---

## 参考与延伸

- 上一节：[Item 36 正确选择算法](item36-choose-right-algorithm.md)
- 回到：[第 5 章 算法](README.md)
