# 第 20 章 新的类型特征

**New Type Traits**

## 本章讲什么

C++20 给 `<type_traits>` 新增的 traits：`is_unbounded_array`、`is_bounded_array`、`remove_cvref`、`is_const_evaluated`、`type_identity`、`unwrap_reference` 等。

## 要点

### 新增的 traits

```cpp
// 1. 数组分类
static_assert(std::is_unbounded_array_v<int[]>);      // 无界数组
static_assert(std::is_bounded_array_v<int[5]>);       // 有界数组
static_assert(!std::is_array_v<double>);

// 2. remove_cvref：去掉 const + volatile + 引用（C++17 只有 remove_cv + remove_reference）
template <typename T>
using Clean = std::remove_cvref_t<T>;   // int& const volatile → int
// C++17 要 std::remove_cv_t<std::remove_reference_t<T>>

// 3. type_identity：身份映射（不推导）
template <typename T>
void foo(std::type_identity_t<T> x);   // 不让 x 参与 T 的推导

// 4. unwrap_reference：解开 reference_wrapper
std::reference_wrapper<int> rw = x;
std::unwrap_reference_t<decltype(rw)> = int&;   // 解出 int&
```

### `type_identity` 的用途

```cpp
// 问题：模板参数推导过于激进
template <typename T>
void foo(T x);    // foo(42) 推导 T = int

// 不想推导 T，要显式指定
template <typename T>
void bar(typename std::type_identity<T>::type x);  // C++14 写法
template <typename T>
void bar(std::type_identity_t<T> x);                // C++20 简写

bar<int>(42);   // OK：T 不从 42 推导
```

### `remove_cvref` 的价值

```cpp
// C++17：要两步
using T1 = std::remove_cv_t<std::remove_reference_t<const int&>>;  // int

// C++20：一步
using T2 = std::remove_cvref_t<const int&>;   // int
```

`remove_cvref` 在泛型编程极常用——`forward`/`move`/`variant` 的 visit 都需要去掉引用和 cv 限定。

### `is_unbounded_array` / `is_bounded_array`

```cpp
template <typename T>
void process(T&& x) {
    if constexpr (std::is_bounded_array_v<std::remove_cvref_t<T>>) {
        // T 是 int[N]，知道大小
        constexpr size_t n = std::extent_v<std::remove_cvref_t<T>>;
    } else if constexpr (std::is_unbounded_array_v<std::remove_cvref_t<T>>) {
        // T 是 int[]，无大小
    }
}
```

区分有界/无界数组，泛型代码能正确处理。

### `<concepts>` 的 traits（见第 5 章）

C++20 的大部分类型约束用 Concepts 而非旧 traits——`is_integral` 用 `std::integral` Concept 替代。但 traits 仍用于编译期逻辑（`if constexpr`）。

## HFT 关联

- **`remove_cvref` 简化泛型**：策略模板的回调参数处理用 `remove_cvref_t` 一步去引用和 const，代码更清爽。
- **`type_identity` 阻止推导**：`process<Strategy>(std::type_identity_t<Strategy> s)` 显式指定策略类型，不推导。
- **数组分类处理**：模板函数接受 `int[]` 和 `int[5]` 时用 `is_bounded_array` 区分，有界数组可编译期知道大小。
- **Concepts 替代 traits 做约束**：`template <integral T>` 比 `enable_if<is_integral>` 简洁，但 traits 仍用于 `if constexpr` 逻辑分支。
- **`unwrap_reference` 解 ref_wrapper**：`vector<reference_wrapper<T>>` 处理时解出真实类型。

## 自测题

1. `remove_cvref` 相比 C++17 的 `remove_cv` + `remove_reference` 好在哪？
2. `type_identity` 的作用？什么时候要阻止模板推导？
3. `is_unbounded_array` 和 `is_bounded_array` 的区别？
4. C++20 中 Concept 和 traits 各自适合什么场景？
5. HFT 泛型代码如何用 `remove_cvref` 简化回调参数处理？

## 代码自测

### Q1: 新类型萃取
```cpp
// C++20 新增 type traits
static_assert(std::is_unbounded_array_v<int[]>);     // true
static_assert(std::is_bounded_array_v<int[5]>);      // true
static_assert(!std::is_unbounded_array_v<int[5]>);   // true

// remove_cvref
static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int>);  // true

// is_constant_evaluated
int compute(int n) {
    if (std::is_constant_evaluated()) {
        // 编译期路径：用简单算法
        return n * 2;
    } else {
        // 运行期路径：可以用内联汇编/快速路径
        return n << 1;
    }
}
```
> `is_constant_evaluated` 解决什么问题？

<details>
<summary>答案与复习指引</summary>

**`std::is_constant_evaluated()`**：在编译期求值时返回 `true`，运行时返回 `false`。

**解决的问题**：同一个函数既要能编译期求值（constexpr），又要在运行时用不同实现（如更快的汇编指令或 `std::sqrt` 等非 constexpr 函数）。

```cpp
constexpr double sqrt_approx(double x) {
    if (std::is_constant_evaluated()) {
        // 编译期：用牛顿迭代法（constexpr 安全）
        double guess = x / 2;
        for (int i = 0; i < 10; ++i) guess = (guess + x/guess) / 2;
        return guess;
    } else {
        // 运行期：用硬件 sqrt 指令（更快）
        return std::sqrt(x);  // 非 constexpr
    }
}
```

**注意**：`is_constant_evaluated()` 在非 constexpr 上下文中总是返回 false。不能用来"检测是否在 constexpr 上下文中"——只在 constexpr 函数内部有意义。

**C++23 增强**：`if consteval {}` 替代 `if (std::is_constant_evaluated())`，更清晰。

**复习：** → [新类型萃取](./README.md)
</details>
