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

<details>
<summary>参考答案</summary>

1. **false**。`std::same_as<T, U>` 要求 T 与 U 是**完全相同的类型**，包含顶层 cv 限定和引用在内；`const int` 与 `int` 是两个不同的类型。
注意它的定义是 `std::is_same_v<T, U> && std::is_same_v<U, T>`（双向都检查），目的是防止用户特化 `is_same` 造成的不对称，保证它真的表达"等价关系"。
需要忽略 cv 时先自己去掉：`std::same_as<std::remove_cv_t<T>, int>`。
2. **包含自身**（对类类型返回 true）。`std::derived_from<D, B>` 定义为 `std::is_base_of_v<B, D> && std::is_convertible_v<const volatile D*, const volatile B*>`，而 `std::is_base_of_v<T, T>` 对类类型是 true（类被视为自身的基类）。
好处：写 `template<std::derived_from<Base> T>` 时，`T` 既可以是派生类也可以就是 `Base` 本身，约束更符合直觉。
3. `std::common_type_t<int, double>` 是 **`double`**。
`std::common_with<T, U>` 的含义正是：存在 `common_type_t<T, U>` 与 `common_reference_t<...>`，且 `T`、`U` 都能**无损**转换到这个公共类型（不会因转换丢信息）。
`int` 与 `double` 的公共类型是 `double`（int 可无损转 double），所以 `std::common_with<int, double>` 成立。
4. 关系是"父与两个互斥的子"：
   - `std::integral<T>`：T 是整数类型（含 `bool`、`char`、各种 int）。
   - `std::signed_integral<T>` = `integral<T> && std::is_signed_v<T>`：有符号整数。
   - `std::unsigned_integral<T>` = `integral<T> && !signed_integral<T>`：无符号整数（**`bool` 算无符号整数类型**）。
所以 `signed_integral` 与 `unsigned_integral` 互斥，二者都蕴含 `integral`；反过来 `integral` 不蕴含其中任何一个。
5. 把它放在模板参数位置或 requires 子句里即可：
```cpp
template <std::convertible_to<double> T>
double to_double(T v) { return static_cast<double>(v); }

template <typename From, typename To>
requires std::convertible_to<From, To>
To convert(From f) { return static_cast<To>(f); }
```
`std::convertible_to<From, To>` 要求：能用 `static_cast<To>` 显式转换，且能**隐式**转换（`is_convertible_v`）到 To。它比 `is_convertible_v` 更严格（还要求 static_cast 合法），是约束"接口可接受哪些输入类型"的常用手段。

</details>
