# 第 22 章 泛型编程的小幅改进

**Small Improvements for Generic Programming**

## 本章讲什么

C++20 对泛型编程的杂项改进：非类型模板参数的 class 类型（第 19 章）、`std::common_reference`、`std::identity`、`std::type_identity`、条件 `explicit`、`std::remove_cvref` 等。

## 要点

### 条件 `explicit`

```cpp
// C++20：explicit(bool) 条件显式
template <typename T>
struct Wrapper {
    template <typename U>
    explicit(!std::is_convertible_v<U, T>)
    Wrapper(U&& u) : value(std::forward<U>(u)) {}
    T value;
};

// 当 U 能隐式转 T 时，构造函数不 explicit（允许隐式）
// 当 U 不能隐式转 T 时，构造函数 explicit（防止意外隐式转换）
```

C++17 要写两个构造函数重载（一个 explicit 一个不 explicit），C++20 一行搞定。

### `std::common_reference`

```cpp
// common_reference：多个类型的公共引用类型
using R = std::common_reference_t<int, double>;  // double
using R2 = std::common_reference_t<int&, double&>;  // double（公共值类型）

// 用于 Ranges：混合类型迭代器的公共类型
```

### `std::type_identity`（见第 20 章）

阻止模板推导的身份映射。

### `std::identity`（C++20）

```cpp
// std::identity：函数对象，返回输入本身
std::identity{}(42);   // 42

// 用途：Ranges 的投影默认值
std::ranges::sort(v, {}, std::identity{});  // 按元素本身排序
std::ranges::sort(v, {}, &Tick::price);     // 按 Tick::price 排序
```

`identity` 是 Ranges 投影的默认值——"按元素本身"。

### `std::forward_like`

```cpp
// C++23 实际引入，C++20 提案
// forward_like<T>(x)：按 T 的值类别转发 x
template <typename T>
void foo(auto&& x) {
    bar(std::forward_like<T>(x));   // 如果 T 是右值，move(x)；否则 forward(x)
}
```

### 模板参数的 ADL 改进

C++20 修复了一些 ADL（参数依赖查找）的边缘情况，让泛型调用更可预测。

## HFT 关联

- **`explicit(bool)` 防意外转换**：`Quantity` 类型构造函数 `explicit(!is_convertible_v<U, T>)`，防止 `int` 隐式变 `Quantity` 但允许同类型。
- **`std::identity` 做默认投影**：策略排序 `ranges::sort(orders, {}, &Order::price)` 按价格，默认 `identity` 按自身。
- **`common_reference` 混合类型算法**：`int` 和 `double` 混合计算时自动推导公共类型。
- **`type_identity` 阻止推导**：泛型函数显式指定模板参数，不让编译器从实参推导。
- **减少重载数量**：`explicit(bool)` 一个构造函数替代两个重载，泛型代码更简洁。

## 自测题

1. `explicit(bool)` 解决什么问题？C++17 怎么实现类似效果？
2. `std::identity` 的作用？在 Ranges 中做什么？
3. `std::common_reference` 的用途？
4. `std::type_identity` 如何阻止模板推导？
5. HFT `Quantity` 类型如何用 `explicit(bool)` 防止意外隐式转换？
