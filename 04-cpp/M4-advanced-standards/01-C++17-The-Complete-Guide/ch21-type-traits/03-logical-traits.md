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
