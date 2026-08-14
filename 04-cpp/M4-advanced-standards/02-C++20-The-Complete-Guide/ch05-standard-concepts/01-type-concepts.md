# 类型关系 Concept

## same_as

```cpp
#include <concepts>

template <typename T, typename U>
concept SameType = std::same_as<T, U>;

static_assert(SameType<int, int>);       // true
static_assert(!SameType<int, const int>); // false
static_assert(!SameType<int, long>);      // false
```

## derived_from

```cpp
class Base {};
class Derived : public Base {};
class Other {};

template <typename T>
concept IsBase = std::derived_from<T, Base>;

static_assert(IsBase<Derived>);  // true
static_assert(!IsBase<Other>);   // false
static_assert(!IsBase<Base>);    // true（derived_from 包含自身）
```

## convertible_to

```cpp
template <typename From, typename To>
concept Convertible = std::convertible_to<From, To>;

static_assert(Convertible<int, double>);      // true
static_assert(Convertible<int, bool>);         // true
static_assert(!Convertible<int, std::string>); // false
```

## common_with / common_reference_with

```cpp
// common_with：两类型有公共类型
static_assert(std::common_with<int, double>);
// std::common_type_t<int, double> = double

// 用于：两类型能混合运算
template <typename T, typename U>
requires std::common_with<T, U>
auto add(T a, U b) {
    using C = std::common_type_t<T, U>;
    return static_cast<C>(a) + static_cast<C>(b);
}

add(1, 2.0);  // C = double → 3.0
```

## 算术 Concept

```cpp
template <typename T> concept Int = std::integral<T>;        // 整数
template <typename T> concept Float = std::floating_point<T>; // 浮点
template <typename T> concept Num = std::arithmetic<T>;       // 算术

// 子分类
std::signed_integral<T>     // 带符号整数
std::unsigned_integral<T>   // 无符号整数

// 使用
template <std::integral T>
T gcd(T a, T b) { /* ... */ }

template <std::floating_point T>
T normalize(T x) { return x / T{1}; }
```

## 自测题

1. `same_as<int, const int>` 是 true 还是 false？为什么？
2. `derived_from` 包含自身吗？（`derived_from<Base, Base>`）
3. `common_with<int, double>` 的 `common_type` 是什么？
4. `integral`、`signed_integral`、`unsigned_integral` 的关系？
5. 如何用 `convertible_to` 约束类型转换？
