# Item 15：尽可能用 constexpr

> 第 3 章 移步现代 C++ · Item 15 · 上一节：[Item 14 noexcept](item14-noexcept.md)

## 这节讲什么

`constexpr` 表示"编译期可求值"。`constexpr` 对象是编译期常量；`constexpr` 函数在编译期能求值时就编译期求值，否则退化为运行时。C++14 起 `constexpr` 函数能力大增。

---

## 核心用法

```cpp
constexpr int square(int x) { return x * x; }     // 编译期求值
constexpr int sz = square(10);                      // sz = 100，编译期确定

// C++14 起 constexpr 函数可用 if/循环/局部变量
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; ++i) result *= i;
    return result;
}
```

`constexpr` 对象 → 编译期常量，可用于模板参数、`static_assert`、数组大小。
`constexpr` 函数 → 至少有一个实参集能在编译期求值；传入运行时值则退化为普通函数。

---

## 新手要点（和 C 的区别）

- **C 用 `#define` / `const`**：C 的常量是 `#define PI 3.14`（无类型）或 `static const double PI = 3.14`（运行时也可能有开销）。C++ 的 `constexpr` 更强——可用于模板参数、`static_assert`。
- **`const` vs `constexpr`**：`const` 表示"不可修改"（但值可能在运行时确定）；`constexpr` 表示"编译期确定"。`constexpr` 蕴含 `const`，反之不然。
- **编译期 ≠ 零开销**：`constexpr` 把计算从运行时搬到编译时，运行时零开销（但编译变慢）。

---

## HFT 关联

- **编译期查表**：协议字段偏移、校验和表、费率表用 `constexpr` 编译期算好，运行时零开销。
- **`static_assert` 编译期校验**：`static_assert(sizeof(Order) == 64);` 确保订单结构体大小符合 cache 行对齐要求。

---

## 自测题

1. `const` 和 `constexpr` 的区别是什么？`constexpr` 蕴含 `const` 吗？
2. C++14 的 `constexpr` 函数比 C++11 强在哪里？
3. `constexpr` 函数传入运行时值会怎样？
4. 为什么 HFT 喜欢用 `constexpr` 做查表？

---

## 参考与延伸

- 下一节：[Item 16 const 线程安全](item16-const-thread-safety.md)
- 回到：[第 3 章 移步现代 C++](README.md)
