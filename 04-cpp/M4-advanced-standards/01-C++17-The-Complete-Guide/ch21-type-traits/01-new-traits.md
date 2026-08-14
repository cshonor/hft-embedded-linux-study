# 新增类型萃取

## is_aggregate：聚合类型检测

```cpp
#include <type_traits>

struct Pod { int x, y; };
class NonPod { public: NonPod() : x(0) {} private: int x; };

static_assert(std::is_aggregate_v<Pod>);       // true：聚合体
static_assert(!std::is_aggregate_v<NonPod>);   // false：有用户定义构造函数
```

**聚合类型**（aggregate）的定义：
- 无用户声明构造函数
- 无 private/protected 非静态数据成员
- 无虚函数
- 无虚基类 / 私有/保护基类

C++17 扩展了聚合体规则（基类可以是聚合体），`is_aggregate` 用于编译期检测。

## has_unique_object_representations

```cpp
struct NoPad { int a; int b; };
struct HasPad { char c; int i; };  // c 后有 3 字节 padding

static_assert(std::has_unique_object_representations_v<NoPad>);   // 可能为 true
// HasPad 有 padding，表示不唯一 → false
```

**含义**：对象的字节表示是否唯一确定——即没有 padding。
- 如果 `true`，可以安全 `memcpy`/`memcmp`。
- `false` 表示有 padding 字节，padding 值不确定，`memcmp` 不可靠。

## invoke_result：替代 result_of

```cpp
// C++11 result_of（C++17 弃用，C++20 移除）
template <typename F, typename... Args>
using R1 = typename std::result_of<F(Args...)>::type;

// C++17 invoke_result
template <typename F, typename... Args>
using R2 = std::invoke_result_t<F, Args...>;
```

**为什么换名字**：
- `result_of<F(Args...)>` 语法怪异（类型拼接语法），且对成员函数指针推导有问题。
- `invoke_result<F, Args...>` 参数分开传，语义更清晰，且与 `std::invoke` 对齐。

## bool_constant

```cpp
// C++11：integral_constant<bool, B>
using True = std::integral_constant<bool, true>;
using False = std::integral_constant<bool, false>;

// C++17：bool_constant 更简洁
using True2 = std::bool_constant<true>;
using False2 = std::bool_constant<false>;

// true_type / false_type 是 bool_constant<true/false> 的别名
static_assert(std::true_type::value == true);
```

## void_t 标准化

```cpp
// C++14 惯用法（void_t 未标准化）
struct voider { template <class...> using void_t = void; };

// C++17：std::void_t 直接可用
template <typename, typename = std::void_t<>>
struct HasValueType : std::false_type {};

template <typename T>
struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type {};

static_assert(HasValueType<std::vector<int>>::value);    // true
static_assert(!HasValueType<int>::value);                 // false
```

`void_t` 的 SFINAE 原理：如果 `T::value_type` 不存在，模板替换失败（SFINAE），落入 `false_type` 基类。

## 自测题

1. `is_aggregate` 检测的条件是什么？有用户定义构造函数的类是聚合体吗？
2. `has_unique_object_representations` 什么时候为 `false`？为什么 padding 导致 `memcmp` 不可靠？
3. `invoke_result` 为什么替代 `result_of`？旧名字有什么问题？
4. `bool_constant<true>` 和 `true_type` 是什么关系？
5. `void_t` 的 SFINAE 原理是什么？检测 `T::value_type` 存在性的写法？
