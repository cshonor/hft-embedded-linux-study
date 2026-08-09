# Item 43：优先传函数对象而非函数指针

> 第 6 章 仿函数与函数 · Item 43 · 上一节：[Item 40-42 函数适配器](item40-42-function-adapters.md) · 下一节：[ch07 使用 STL 编程](../ch07-programming-with-stl/README.md)

## 为什么要学这个（先建立直觉）

在 C 里，`qsort` 接收函数指针。编译器看到函数指针调用时，通常**无法内联**——因为它不确定指针指向哪个函数（即使只有一个候选）。

```c
/* C: 函数指针 → 无法内联 */
int cmp(const void *a, const void *b) { return *(int*)a - *(int*)b; }
qsort(arr, n, sizeof(int), cmp);
// 编译器：cmp 是指针，可能指向任何函数 → 不内联
```

```cpp
// C++: 仿函数 → 类型唯一 → 编译器可内联
struct Cmp {
    bool operator()(int a, int b) const { return a < b; }
};
std::sort(v.begin(), v.end(), Cmp{});  // Cmp{} 类型已知 → 内联！

// C++11: lambda 本质就是编译器生成的仿函数
std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });  // 也可内联
```

**直觉**：函数指针是运行时间接调用，仿函数/lambda 是编译期已知类型——编译器可以内联，性能差距可达数倍。

## 这节讲什么

### 为什么函数指针阻碍内联

```cpp
// 函数指针版本
bool less_than(int a, int b) { return a < b; }
std::sort(v.begin(), v.end(), less_than);
// 编译器看到 less_than 是一个指针参数
// 它不能假设指针只指向 less_than（理论上可被重新赋值）
// → 保守起见，不内联 → 每次比较一次函数调用
```

```cpp
// 仿函数版本
struct LessThan {
    bool operator()(int a, int b) const { return a < b; }
};
std::sort(v.begin(), v.end(), LessThan{});
// 模板实例化：sort<decltype(v.begin()), LessThan>
// LessThan::operator() 类型已知，调用点确定
// → 编译器内联 → 零调用开销
```

### 性能对比

```cpp
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>

bool func_ptr_cmp(int a, int b) { return a < b; }
struct FunctorCmp {
    bool operator()(int a, int b) const { return a < b; }
};

int main() {
    std::vector<int> v(1000000);
    for (auto& x : v) x = rand();

    auto v1 = v;
    auto t1 = std::chrono::high_resolution_clock::now();
    std::sort(v1.begin(), v1.end(), func_ptr_cmp);  // 函数指针
    auto t2 = std::chrono::high_resolution_clock::now();

    auto v2 = v;
    auto t3 = std::chrono::high_resolution_clock::now();
    std::sort(v2.begin(), v2.end(), FunctorCmp{});  // 仿函数
    auto t4 = std::chrono::high_resolution_clock::now();

    std::cout << "func ptr: " << (t2-t1).count() << " ns\n";
    std::cout << "functor:  " << (t4-t3).count() << " ns\n";
    // 典型结果：functor 快 1.5-3x
}
```

### lambda = 编译器生成的仿函数

```cpp
// 这个 lambda
auto cmp = [](int a, int b) { return a < b; };

// 编译器生成等价的仿函数
struct __lambda_cmp {
    bool operator()(int a, int b) const { return a < b; }
};
auto cmp = __lambda_cmp{};
```

所以 lambda 和手写仿函数**性能完全等价**，但写法更简洁。

## 常见错误（新手踩坑）

### 错误 1：热路径用函数指针

```cpp
// 反模式：热路径用函数指针
typedef bool (*CompareFn)(const Order&, const Order&);
void process_orders(std::vector<Order>& orders, CompareFn cmp) {
    std::sort(orders.begin(), orders.end(), cmp);  // 不内联！
}
```

**修复**：用模板参数或 lambda。

```cpp
// 模板版本
template<typename Cmp>
void process_orders(std::vector<Order>& orders, Cmp cmp) {
    std::sort(orders.begin(), orders.end(), cmp);  // 可内联
}
// 调用：process_orders(orders, [](const Order& a, const Order& b) { ... });
```

### 错误 2：用 std::function 存热路径回调

```cpp
// std::function 有类型擦除开销
void process(std::vector<int>& v, std::function<bool(int,int)> cmp) {
    std::sort(v.begin(), v.end(), cmp);  // 不内联（间接调用）
}
```

**修复**：模板参数。

```cpp
template<typename Cmp>
void process(std::vector<int>& v, Cmp cmp) {
    std::sort(v.begin(), v.end(), cmp);  // 可内联
}
```

### 错误 3：以为函数指针和仿函数性能一样

```cpp
// 函数指针和仿函数"功能相同"但性能不同
std::sort(v.begin(), v.end(), func_ptr_cmp);  // 慢
std::sort(v.begin(), v.end(), FunctorCmp{});  // 快
std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });  // 快（=仿函数）
```

## 新手要点（和 C 的区别）

| 方面 | C (函数指针) | C++ (仿函数/lambda) |
|------|-------------|---------------------|
| 类型 | 统一的指针类型 | 每个仿函数/lambda 类型唯一 |
| 内联 | 难（间接调用） | 可内联（类型已知） |
| 状态 | 无（全局变量变通） | 有（成员变量 / 捕获） |
| 语法 | 简洁 | 仿函数繁琐，lambda 简洁 |
| 性能 | 最差 | 最好 |

## HFT 关联

- **热路径排序比较器必须用 lambda**：`std::sort` 配函数指针不内联，配 lambda 可内联 + SIMD 优化
- **模板参数传策略**：策略模式用模板参数 `template<typename Strategy>` 而非 `std::function`，确保编译期内联
- **lambda 捕获注意拷贝**：按值捕获大对象增加仿函数大小，热路径用引用捕获（注意线程安全）

## 代码自测

### Q1: 内联判断

```cpp
bool cmp_func(int a, int b) { return a < b; }
struct CmpFunctor {
    bool operator()(int a, int b) const { return a < b; }
};

std::sort(v.begin(), v.end(), cmp_func);    // A
std::sort(v.begin(), v.end(), CmpFunctor{}); // B
std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; }); // C
```
> A、B、C 哪个能内联？

<details>
<summary>答案</summary>

**B 和 C 能内联**，A 通常不能。

- **A（函数指针）**：`cmp_func` 作为指针传入，编译器不确定指针指向哪个函数 → 不内联
- **B（仿函数）**：`CmpFunctor` 类型已知，`operator()` 可内联
- **C（lambda）**：编译器生成唯一闭包类型，等价于 B，可内联

B 和 C 性能等价，C 写法更简洁。
</details>

### Q2: 模板 vs std::function

```cpp
// A: std::function 参数
void sort_v1(std::vector<int>& v, std::function<bool(int,int)> cmp);

// B: 模板参数
template<typename Cmp>
void sort_v2(std::vector<int>& v, Cmp cmp);
```
> A 和 B 在 sort 内联上有什么区别？

<details>
<summary>答案</summary>

**B 可内联，A 不可**。

- **A（std::function）**：类型擦除 → `operator()` 通过虚表/函数指针间接调用 → 不内联。且可能堆分配。
- **B（模板参数）**：每个 `Cmp` 类型生成独立实例化 → 调用点类型已知 → 可内联

**HFT**：热路径用模板参数。`std::function` 用于回调注册表等"需要存储异质可调用对象"的非热路径场景。
</details>

### Q3: lambda 捕获大小

```cpp
int threshold = 100;
std::vector<int> buf(1024);

// A: 按值捕获 threshold
auto a = [threshold](int x) { return x > threshold; };

// B: 按引用捕获 buf
auto b = [&buf](int x) { /* use buf */ };

// C: 按值捕获 buf
auto c = [buf](int x) { /* use buf */ };
```
> A、B、C 的仿函数大小分别是多少？

<details>
<summary>答案</summary>

- **A**：sizeof = 4（一个 int）。轻量，拷贝廉价。
- **B**：sizeof = 8（一个指针）。引用捕获只存指针。
- **C**：sizeof = 4096（拷贝整个 vector）。**每次拷贝 4KB**！

**教训**：大对象用引用捕获 `[&buf]`，按值捕获只用于小类型（int/double/指针）。

**HFT**：热路径仿函数必须轻量，sort 内部会拷贝仿函数。大捕获用引用（注意生命周期和线程安全）。
</details>

### Q4: 有状态 lambda

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
int comparisons = 0;
std::sort(v.begin(), v.end(), [&comparisons](int a, int b) {
    comparisons++;
    return a < b;
});
std::cout << comparisons;
```
> 这段代码能正确统计比较次数吗？有什么风险？

<details>
<summary>答案</summary>

**能正确统计**。lambda 引用捕获 `comparisons`，每次比较递增的是外部变量。

**风险**：
1. **线程安全**：如果 sort 被并行执行（C++17 `std::execution::par`），多线程同时写 `comparisons` 是数据竞争 → UB
2. **sort 可能拷贝 lambda**：但引用捕获只拷贝指针，指向同一个 `comparisons`，所以统计正确

**HFT**：热路径避免有状态 lambda（副作用影响优化）。如果必须统计，用无竞争的原子计数器，且不在生产热路径上。
</details>

## 参考与延伸

- 上一节：[Item 40-42 函数适配器](item40-42-function-adapters.md)
- 下一节：[ch07 使用 STL 编程](../ch07-programming-with-stl/README.md)
- [Effective Modern C++ Item 34：lambda 优于 bind](../../M1-modern-cpp/01-Effective-Modern-C++/ch06-lambda-expressions/README.md)
