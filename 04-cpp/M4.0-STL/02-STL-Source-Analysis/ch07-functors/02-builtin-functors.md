# 7.2 STL 内置仿函数
> 第 7 章 仿函数 · 第 2 节 · 上一节：[7.1 两大基类](01-base-classes.md) · 下一节：[7.3 可配对仿函数](03-composable-functors.md)

## 为什么要学这个（先建立直觉）

C 里要传"比较函数"给 `qsort`，你得手写函数：

```c
// C: 每种比较都得手写函数
int cmp_int_asc(const void* a, const void* b) { return *(int*)a - *(int*)b; }
int cmp_int_desc(const void* a, const void* b) { return *(int*)b - *(int*)a; }
qsort(arr, n, sizeof(int), cmp_int_asc);
// 没有内置的 plus/minus/less/greater
```

C++ STL 内置了三大类仿函数，开箱即用：

```cpp
std::sort(v.begin(), v.end(), std::greater<int>{});  // 降序排序
auto sum = std::accumulate(v.begin(), v.end(), 0, std::plus<int>{});  // 求和
// 不用手写，直接用
```

理解内置仿函数的命名和用途，你才能读懂用了它们的代码（包括 STL 源码自身）。

## 这节讲什么

STL 预置三大类仿函数：算术、关系、逻辑，全部继承 `unary_function`/`binary_function`。

### 算术仿函数

```cpp
// 全部继承 binary_function<T, T, T>
template<class T>
struct plus : binary_function<T, T, T> {
    T operator()(const T& x, const T& y) const { return x + y; }
};

template<class T>
struct minus : binary_function<T, T, T> {
    T operator()(const T& x, const T& y) const { return x - y; }
};

template<class T>
struct multiplies : binary_function<T, T, T> {
    T operator()(const T& x, const T& y) const { return x * y; }
};

template<class T>
struct divides : binary_function<T, T, T> {
    T operator()(const T& x, const T& y) const { return x / y; }
};

template<class T>
struct modulus : binary_function<T, T, T> {
    T operator()(const T& x, const T& y) const { return x % y; }
};

template<class T>
struct negate : unary_function<T, T> {
    T operator()(const T& x) const { return -x; }
};
```

| 仿函数 | 操作 | 用途示例 |
|--------|------|---------|
| `plus<T>` | `x + y` | `accumulate` 求和 |
| `minus<T>` | `x - y` | 差值计算 |
| `multiplies<T>` | `x * y` | `accumulate` 求积 |
| `divides<T>` | `x / y` | 比率计算 |
| `modulus<T>` | `x % y` | 取模 |
| `negate<T>` | `-x` | 取反（一元） |

### 关系仿函数

```cpp
template<class T>
struct less : binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const { return x < y; }
};

template<class T>
struct equal_to : binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const { return x == y; }
};
// 还有 greater, less_equal, greater_equal, not_equal_to
```

| 仿函数 | 操作 | 用途示例 |
|--------|------|---------|
| `less<T>` | `x < y` | `sort` 升序（默认）、`map`/`set` 排序 |
| `greater<T>` | `x > y` | `sort` 降序、`priority_queue` 小顶堆 |
| `equal_to<T>` | `x == y` | `unordered_map` 键相等（默认） |
| `less_equal<T>` | `x <= y` | 范围判断 |
| `greater_equal<T>` | `x >= y` | 范围判断 |
| `not_equal_to<T>` | `x != y` | 不等判断 |

### 逻辑仿函数

```cpp
template<class T>
struct logical_and : binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const { return x && y; }
};

template<class T>
struct logical_or : binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const { return x || y; }
};

template<class T>
struct logical_not : unary_function<T, bool> {
    bool operator()(const T& x) const { return !x; }
};
```

| 仿函数 | 操作 | 用途示例 |
|--------|------|---------|
| `logical_and<T>` | `x && y` | 谓词组合 |
| `logical_or<T>` | `x \|\| y` | 谓词组合 |
| `logical_not<T>` | `!x` | 谓词取反（一元） |

### 实际用法

```cpp
// sort 降序
std::sort(v.begin(), v.end(), std::greater<int>{});

// priority_queue 小顶堆（最小值在顶）
std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;

// accumulate 求积
auto product = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>{});

// transform 取反
std::transform(v.begin(), v.end(), v.begin(), std::negate<int>{});
```

## 常见错误（新手踩坑）

### 错误 1：C++14 忘写模板参数

```cpp
// C++11: 必须指定类型
std::sort(v.begin(), v.end(), std::less<int>{});  // OK

// C++14+: 可以用透明仿函数（less<>）
std::sort(v.begin(), v.end(), std::less<>{});  // C++14 OK，自动推导

// C++11: less<> 不支持
// std::less<>{};  // C++11 编译错误
```

### 错误 2：用函数指针替代仿函数

```cpp
// ❌ 函数指针阻碍内联
bool greater_than(int a, int b) { return a > b; }
std::sort(v.begin(), v.end(), greater_than);  // 间接调用，可能不内联

// ✅ 仿函数可内联
std::sort(v.begin(), v.end(), std::greater<int>{});  // 类型已知，可内联
```

### 错误 3：以为 greater 是大于等于

```cpp
// ❌ greater 是严格大于（>），不是大于等于（>=）
std::sort(v.begin(), v.end(), std::greater<int>{});
// 严格降序：5,4,3,2,1
// 不是 5,5,4,4,3,3...

// 大于等于用 greater_equal
std::sort(v.begin(), v.end(), std::greater_equal<int>{});  // 但排序用 <= 有问题
// 注意：排序比较器必须严格弱序，不能用 >=
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 每种操作手写函数 | 内置 plus/less/greater 等 | C++ 开箱即用 |
| 函数指针不可内联 | 仿函数可内联 | C++ 更快 |
| `void*` 无类型安全 | 模板编译期检查 | C++ 安全 |
| 无逻辑组合 | `logical_and`/`logical_or` | C++ 可组合 |

## HFT 关联

- **`greater` 做小顶堆**：`priority_queue<T, vector<T>, greater<T>>` 是定时器/事件队列的常用模式
- **`less` 是默认排序**：`map`/`set`/`sort`/`priority_queue` 默认用 `less<T>`，理解它才能理解默认行为
- **C++14 透明仿函数**：`less<>{}` 让异构比较无需指定类型，减少类型冗余
- **新代码用 lambda**：`[](int a, int b) { return a > b; }` 比 `std::greater<int>{}` 更直观

## 代码自测

### Q1: less 和 greater 分别做什么？

```cpp
std::priority_queue<int> pq1;  // 默认 less<int>
std::priority_queue<int, std::vector<int>, std::greater<int>> pq2;

pq1.push(3); pq1.push(1); pq1.push(5);
pq2.push(3); pq2.push(1); pq2.push(5);

pq1.top();  // ?
pq2.top();  // ?
```
> pq1 和 pq2 的 top 分别是什么？

<details>
<summary>答案与复习指引</summary>

- `pq1.top()` = **5**（大顶堆，`less` 意味着"父 < 子"不成立 → 父 ≥ 子 → 最大值在顶）
- `pq2.top()` = **1**（小顶堆，`greater` 意味着"父 > 子"不成立 → 父 ≤ 子 → 最小值在顶）

**记忆方法**：比较器定义的是"子节点是否应该上浮"——`less` 时子大于父上浮 → 最大值浮到顶；`greater` 时子小于父上浮 → 最小值浮到顶。

**HFT**：
- 定时器队列用小顶堆（`greater`），最近到期的事件在顶
- 延迟统计用大顶堆（`less`），最大延迟在顶（但求 P99 用 `nth_element` 更高效）

**复习：** → [关系仿函数](./02-builtin-functors.md)
</details>

### Q2: 为什么仿函数比函数指针快？

```cpp
// 方式 A: 函数指针
bool cmp(int a, int b) { return a > b; }
std::sort(v.begin(), v.end(), cmp);

// 方式 B: 仿函数
std::sort(v.begin(), v.end(), std::greater<int>{});
```
> A 和 B 在编译器优化层面有什么区别？

<details>
<summary>答案与复习指引</summary>

**方式 A（函数指针）**：
- `sort` 接收的是 `bool(*)(int, int)` 指针
- 每次比较通过指针间接调用 → 可能不内联
- 即使编译器能内联（LTO/PGO），也不保证

**方式 B（仿函数）**：
- `sort` 接收的是 `greater<int>` 类型对象
- `operator()` 是成员函数，类型已知 → 编译器可内联
- 内联后比较操作变成直接的 `a > b` 指令

**性能差距**：函数指针版本可能慢 2-5x（间接跳转 + 无法内联），数据量大时明显。

**C++11 lambda**：
```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
// lambda 闭包类型已知 → 可内联 → 和仿函数一样快
```

**HFT**：热路径排序用仿函数或 lambda，不用函数指针。

**复习：** → [仿函数 vs 函数指针](./02-builtin-functors.md)
</details>

### Q3: C++14 的 transparent comparators 是什么？

```cpp
// C++11: 必须指定类型
std::set<int, std::less<int>> s1;
s1.find(42);  // 查找 int 42

// C++14: 透明比较器
std::set<int, std::less<>> s2;  // less<> 无类型参数
// s2.find(42L);  // 可以用 long 查找 int！无需构造临时 int
```
> less<> 和 less<int> 有什么区别？

<details>
<summary>答案与复习指引</summary>

**`less<int>`**：
- 类型固定为 `int`
- `find(42L)` 需要把 `long` 转为 `int` 再查找（可能构造临时对象）

**`less<>`（透明比较器，C++14）**：
- 无类型参数，调用时推导
- `find(42L)` 直接用 `long` 和 `int` 比较（`operator<(long, int)`）
- 避免构造临时对象，对异构查找更高效

**实际收益**：
```cpp
std::set<std::string, std::less<>> s;
// 用 const char* 查找，不用构造临时 string
s.find("hello");  // 直接比较 const char* 和 string
```

**HFT**：异构查找避免临时对象构造，减少热路径开销。

**复习：** → [C++14 忘写模板参数](./02-builtin-functors.md)
</details>

### Q4: 下面的代码输出什么？

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto product = std::accumulate(v.begin(), v.end(), 1,
    std::multiplies<int>{});
std::cout << product;  // ?
```
> multiplies 在 accumulate 中起什么作用？

<details>
<summary>答案与复习指引</summary>

**输出 120**（1×2×3×4×5 = 120）。

`multiplies<int>` 把 `accumulate` 的默认 `+` 替换为 `×`：
```
init = 1
1 × 1 = 1
1 × 2 = 2
2 × 3 = 6
6 × 4 = 24
24 × 5 = 120
```

初值必须是 1（乘法单位元），不是 0（0 会让结果永远是 0）。

**HFT**：计算价格变动幅度的连续乘积（复利）用 `multiplies`。但注意浮点精度——大量乘法可能累积误差。

**复习：** → [算术仿函数](./02-builtin-functors.md)
</details>

## 参考与延伸

- 上一节：[7.1 两大基类](01-base-classes.md)
- 下一节：[7.3 可配对仿函数](03-composable-functors.md)
- 源码参考：`bits/stl_function.h`（所有内置仿函数定义）
