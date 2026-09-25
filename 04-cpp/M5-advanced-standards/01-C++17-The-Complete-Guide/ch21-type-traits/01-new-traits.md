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

<details>
<summary>参考答案</summary>

1. 按 C++17 定义，**聚合体**（aggregate）要求：无用户声明或继承的构造函数、无 private 或 protected 非静态数据成员、无虚函数、无虚基类或私有/保护基类。`is_aggregate<T>` 为 `true` 当且仅当 `T`（数组则看其元素类型）满足这一组条件。
所以**有用户定义构造函数的类不是聚合体**，`is_aggregate_v<NonPod>` 为 `false`，也就不能用聚合初始化 `T{...}` 逐成员赋值。
2. 该 trait 为 `true` 的前提是：`T` 是 trivially copyable，且任意两个值相等的对象具有相同的**对象表示**（object representation）；否则为 `false`。
典型的 `false` 情形：结构体里有 padding（`char c; int i;` 中间 3 字节）、含 `bool` 或位域等存在多余位表示的类型、浮点类型（NaN、±0 有多种字节表示）。
padding 导致 `memcmp` 不可靠，是因为 padding 字节的内容**未指定**（编译器不保证清零），两个逻辑上相等的对象 padding 字节可能不同，`memcmp` 却会判为不等。
3. `result_of<F(Args...)>` 用「函数类型拼接」的语法把可调用物和参数拼成一个类型，写起来别扭；而且在参数是引用/右值引用、`F` 是成员函数指针等场景下推导规则几经修补，语义不够清晰。
`invoke_result<F, Args...>` 参数分开传，直接对应 `std::invoke` 的 **INVOKE** 语义，语义明确，也能覆盖成员函数指针和数据成员指针。
`result_of` 在 C++17 被弃用、C++20 被移除，新代码一律写 `std::invoke_result_t<F, Args...>`。
4. C++17 起 `std::bool_constant<B>` 就是 `std::integral_constant<bool, B>` 的别名模板，而 `std::true_type` 被定义为 `std::bool_constant<true>`、`std::false_type` 为 `std::bool_constant<false>`。
所以 `bool_constant<true>` 和 `true_type` 是**同一个类型**（`bool_constant<false>` 与 `false_type` 同理）。写 `bool_constant<B>` 的好处是能在泛型代码里直接用布尔值参数化，不必再套 `integral_constant<bool, B>`。
5. `std::void_t<Args...>` 是别名模板，展开后恒为 `void`；关键在于它**在模板实参替换的语境中**展开，所以若实参（如 `typename T::value_type`）不存在，替换失败属于 SFINAE（软错误），该偏特化被静默丢弃，匹配回主模板。
检测 `T::value_type` 存在性的标准写法：
```cpp
template <typename, typename = std::void_t<>>
struct HasValueType : std::false_type {};

template <typename T>
struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type {};
```
`std::vector<int>` 有 `value_type` 于是命中偏特化为 `true`；`int` 没有则 SFINAE 落到主模板为 `false`。

</details>
