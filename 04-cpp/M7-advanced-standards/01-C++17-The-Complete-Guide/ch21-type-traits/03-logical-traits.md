# conjunction / disjunction / negation

## 三个逻辑元函数

```cpp
#include <type_traits>

// conjunction：编译期逻辑与（&&），短路求值
template <typename... Ts>
constexpr bool all_integral = std::conjunction_v<std::is_integral<Ts>...>;

static_assert(all_integral<int, long, char>);      // true
static_assert(!all_integral<int, double, char>);    // false（double 不是整数）

// disjunction：编译期逻辑或（||），短路求值
template <typename... Ts>
constexpr bool any_float = std::disjunction_v<std::is_floating_point<Ts>...>;

static_assert(any_float<int, double, char>);        // true（double 是浮点）
static_assert(!any_float<int, long, char>);          // false

// negation：编译期逻辑非（!）
template <typename T>
constexpr bool not_ptr = std::negation_v<std::is_pointer<T>>;

static_assert(not_ptr<int>);       // true（int 不是指针）
static_assert(!not_ptr<int*>);     // false（int* 是指针）
```

## 短路求值

```cpp
// conjunction 短路：遇到 false_type 就停止
// 不会实例化后面的 traits——避免编译错误
template <typename T>
void process(T x) {
    // 如果 T 没有 ::value_type，后面的 is_integral 不会实例化
    static_assert(
        std::conjunction_v<
            std::is_integral<typename T::value_type>  // 如果这里失败，短路
        >,
        "T::value_type must be integral"
    );
}
```

**短路保证**：`conjunction` 和 `disjunction` 在编译期短路求值，即使后面的 trait 会编译错误也不会实例化。这是手写 `is_integral<T1>::value && is_integral<T2>::value` 做不到的——后者会全部实例化。

## 底层实现

```cpp
// conjunction 的简化实现
template <typename...> struct conjunction : std::true_type {};
template <typename B> struct conjunction<B> : B {};
template <typename B, typename... Bs>
struct conjunction<B, Bs...>
    : std::conditional_t<bool(B::value), conjunction<Bs...>, B> {};
// 如果 B::value 为 true，递归检查 Bs...
// 如果 B::value 为 false，直接继承 B（短路）
```

## 实际应用

```cpp
// 约束所有模板参数都是算术类型
template <typename... Ts>
constexpr bool all_arithmetic = std::conjunction_v<std::is_arithmetic<Ts>...>;

template <typename... Ts,
          typename = std::enable_if_t<all_arithmetic<Ts...>>>
auto sum(Ts... ts) { return (ts + ...); }  // 折叠表达式

// 约束至少有一个参数是指针
template <typename... Ts,
          typename = std::enable_if_t<std::disjunction_v<std::is_pointer<Ts>...>>>
void process_ptrs(Ts... ts) { /* ... */ }
```

## 自测题

1. `conjunction` 和 `disjunction` 分别对应什么逻辑运算？
2. 短路求值为什么重要？手写 `&&` 能短路吗？
3. `negation<is_pointer<T>>` 等价于什么？
4. 用 `conjunction` 写一个约束"所有参数都是 trivially_copyable"的模板。
5. `conjunction` 的底层实现用了什么技巧来实现短路？

<details>
<summary>参考答案</summary>

1. `std::conjunction<Bs...>` 对应逻辑**与**（`&&`），全部为 `true` 才是 `true`；`std::disjunction<Bs...>` 对应逻辑**或**（`||`），至少一个 `true` 即为 `true`；配套的 `std::negation<B>` 对应逻辑**非**（`!`）。
它们操作的都是 trait（有 `::value` 的类型），结果本身也是一个 `bool_constant`，可以取 `::value` 或用 `_v` 变量模板。
2. 短路的重要性在于**避免实例化后面的 trait**。例如要先确认 `T::value_type` 存在，再判断它是否整型：手写 `is_integral_v<T> && is_integral_v<typename T::value_type>` 时两侧都会被实例化，`T` 没有 `value_type` 就是**硬编译错误**。
普通 `&&` 是表达式求值，所有操作数都要先实例化/求值，不存在模板层面的短路；`conjunction` 则保证一旦遇到 `false` 就不再实例化后续 trait，从而把硬错误变成「返回 false」。
3. `std::negation<std::is_pointer<T>>` 等价于 `!std::is_pointer_v<T>`。
标准把 `negation<B>` 定义为派生自 `bool_constant<!bool(B::value)>`，即把 `B` 的布尔值取反；C++17 同时提供 `negation_v<B>`，所以 `std::negation_v<std::is_pointer<T>>` 就是 `!std::is_pointer_v<T>`。
4. 用 `conjunction_v` 折叠参数包即可：
```cpp
template <typename... Ts>
constexpr bool all_trivially_copyable =
    std::conjunction_v<std::is_trivially_copyable<Ts>...>;

template <typename... Ts,
          typename = std::enable_if_t<all_trivially_copyable<Ts...>>>
void bulk_copy(Ts&&... ts) { /* 可走 memcpy 快路径 */ }
```
注意空包时 `conjunction_v<>` 为 `true`（与「所有元素都满足」的空真一致），`disjunction_v<>` 为 `false`。
5. 核心是**递归继承 + `conditional_t` 的惰性实例化**：
```cpp
template <typename...> struct conjunction : std::true_type {};
template <typename B> struct conjunction<B> : B {};
template <typename B, typename... Bs>
struct conjunction<B, Bs...>
  : std::conditional_t<bool(B::value), conjunction<Bs...>, B> {};
```
`std::conditional` 只会「选中」其中一个类型，另一个不会被实例化；所以当 `B::value` 为 `false` 时 `conjunction<Bs...>` 根本不会被实例化，后面的 trait 也就不会被求值，这就是短路（`disjunction` 是镜像写法，命中 `true` 时停止）。

</details>
