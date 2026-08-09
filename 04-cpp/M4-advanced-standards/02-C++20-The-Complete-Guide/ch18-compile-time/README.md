# 第 18 章 编译时计算

**Compile-Time Computing**

## 本章讲什么

C++20 大幅扩展编译期计算能力：`consteval`（必须编译期）、`constinit`（编译期初始化但非 const）、`constexpr` 改进（可分配内存、可 try-catch、可 goto）、`is_constant_evaluated`。

## 要点

### `consteval`：必须编译期执行

```cpp
// consteval：函数必须编译期调用，不能运行期
consteval int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

constexpr int x = factorial(5);   // OK：编译期
// int y = factorial(rand());     // 错误：运行期值不能调 consteval

// vs constexpr（可选编译期）
constexpr int maybe_compile_time(int n) { return n * 2; }
int z = maybe_compile_time(rand());  // OK：运行期也行
```

`consteval` 比 `constexpr` 更严——保证编译期执行，可用于编译期表生成、断言。

### `constinit`：编译期初始化但非 const

```cpp
// constinit：编译期初始化，但运行期可改
constinit int counter = 0;   // 编译期初始化，避免静态初始化顺序问题

void inc() { counter++; }    // 运行期可改

// vs constexpr
constexpr int x = 5;          // 编译期初始化 + const（不能改）
// vs const
const int y = foo();          // 运行期初始化（如果 foo 非 constexpr）
```

`constinit` 解决**静态初始化顺序灾难**（static init order fiasco）——保证变量在编译期初始化，不依赖运行期初始化顺序。

### `constexpr` 的扩展

C++20 让 `constexpr` 函数能做更多：

```cpp
// 1. 可分配内存（std::vector constexpr）
constexpr int sum_vec() {
    std::vector<int> v = {1, 2, 3, 4, 5};   // C++20 constexpr vector
    int s = 0;
    for (int x : v) s += x;
    return s;
}
constexpr int s = sum_vec();   // 编译期执行，vector 在编译期分配/释放

// 2. 可 try-catch
constexpr int foo() {
    try { /* ... */ }
    catch (...) { /* 编译期异常不抛，但语法允许 */ }
    return 0;
}

// 3. 可 goto
constexpr int bar() {
    int s = 0;
    int i = 0;
start:
    if (i < 10) { s += i; ++i; goto start; }
    return s;
}
```

### `std::is_constant_evaluated`

```cpp
constexpr int compute(int x) {
    if (std::is_constant_evaluated()) {
        // 编译期路径：用编译期友好的算法
        return compile_time_algo(x);
    } else {
        // 运行期路径：可用 SIMD/intrinsics
        return runtime_fast_algo(x);
    }
}

int a = compute(10);            // 运行期：runtime_fast_algo
constexpr int b = compute(10);  // 编译期：compile_time_algo
```

让同一个函数在编译期和运行期走不同路径——编译期用可移植的纯算法，运行期用 SIMD/intrinsics 加速。

### `constexpr` 容器

C++20 让 `std::vector`/`std::string` 的部分操作 `constexpr`——可在编译期构造/操作容器，返回值用于编译期常量。

## HFT 关联

- **`consteval` 生成参数表**：策略参数表用 `consteval` 函数编译期生成，运行期零开销。
- **`constinit` 避免静态初始化顺序问题**：全局配置/计数器用 `constinit`，保证编译期初始化，不依赖链接顺序。
- **`is_constant_evaluated` 双路径**：信号计算编译期用纯算法、运行期用 AVX/SIMD，同一函数两套实现。
- **`constexpr` 容器做编译期表**：合约表、手续费率表用 `constexpr vector` 编译期构造，运行期只读。
- **`consteval` 断言**：`consteval` 函数里 `static_assert` 编译期强制不变式，HFT 配置参数编译期校验。
- **C++20 `constexpr` 内存分配限制**：编译期分配的内存必须在编译期释放（`constexpr` 函数结束前），不能把编译期 vector 直接给运行期——要转 `array` 或 `span`。

## 自测题

1. `consteval` 和 `constexpr` 的区别？各自什么时候用？
2. `constinit` 解决什么问题？和 `constexpr`/`const` 的区别？
3. `is_constant_evaluated` 的作用？为什么需要双路径？
4. C++20 `constexpr` 函数能做哪些 C++17 不能的事？
5. HFT 如何用 `is_constant_evaluated` 让同一函数编译期纯算法、运行期 SIMD？
