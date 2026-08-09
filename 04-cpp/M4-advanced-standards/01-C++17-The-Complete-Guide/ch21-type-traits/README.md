# 第 21 章 type_traits 扩展

**Extensions of Type Traits**

## 本章讲什么

C++17 给 `<type_traits>` 加了一批新的类型查询和变换工具：`is_aggregate`、`has_unique_object_representations`、`invoke_result`、`bool_constant`、`void_t` 的标准化等。

## 要点

### 新增的 traits

```cpp
// 1. is_aggregate：是否聚合类型
struct Pod { int x, y; };
class NonPod { public: NonPod(){} private: int x; };
static_assert(std::is_aggregate_v<Pod>);        // true
static_assert(!std::is_aggregate_v<NonPod>);    // false

// 2. has_unique_object_representations：能否安全 memcpy/比较
static_assert(std::has_unique_object_representations_v<int>);   // true
// 有 padding 的结构可能 false

// 3. invoke_result：替代 result_of（C++17 弃用 result_of）
template <typename F, typename... Args>
using R = std::invoke_result_t<F, Args...>;

// 4. bool_constant：true_type/false_type 的泛化
template <bool B>
using Bool = std::bool_constant<B>;   // 等价 integral_constant<bool, B>

// 5. void_t（C++17 标准化，源自 C++14 SFINAE 惯用法）
template <typename, typename = std::void_t<>>
struct HasMember : std::false_type {};
template <typename T>
struct HasMember<T, std::void_t<decltype(T::member)>> : std::true_type {};
```

### `is_invocable` 系列

```cpp
// 检查能否用指定参数调用
static_assert(std::is_invocable_v<decltype(foo), int>);

// 检查返回类型
static_assert(std::is_invocable_r_v<bool, decltype(pred), int>);  // 返回 bool?
```

### `conjunction`/`disjunction`/`negation`

```cpp
// 逻辑组合（编译期 &&/||/!）
template <typename... Ts>
constexpr bool all_integral = std::conjunction_v<std::is_integral<Ts>...>;

template <typename... Ts>
constexpr bool any_float = std::disjunction_v<std::is_floating_point<Ts>...>;

template <typename T>
constexpr bool not_ptr = std::negation_v<std::is_pointer<T>>;
```

### 变量模板（C++17 标准化）

C++17 给几乎所有 traits 加了 `_v` 变量模板后缀：

```cpp
// C++14
std::is_integral<T>::value
// C++17
std::is_integral_v<T>
```

## HFT 关联

- **`is_aggregate` 保证 POD**：模板里 `static_assert(is_aggregate_v<T>)` 确保 tick/order 结构是 POD，可 memcpy。
- **`has_unique_object_representations`**：判断能否用 `memcmp`/`memcpy` 快速比较/拷贝，热路径优化依据。
- **`is_invocable` 检查回调签名**：策略注册时 `static_assert(is_invocable_r_v<bool, CB, Tick>)` 编译期保证回调签名正确。
- **`conjunction` 批量约束**：`static_assert(conjunction_v<is_arithmetic<Ts>...>)` 确保所有模板参数是数值类型。
- **`invoke_result` 推导返回类型**：泛型封装层用 `invoke_result_t<F, Args...>` 推导回调返回类型。
- **`_v` 变量模板**：代码更简洁，`is_trivially_copyable_v<T>` 比 `::value` 清爽。

## 自测题

1. `is_aggregate` 和 `is_trivially_copyable` 的区别？HFT 各自用来保证什么？
2. `has_unique_object_representations` 什么意思？有什么用？
3. `invoke_result` 替代了什么？为什么旧的名字被弃用？
4. `conjunction`/`disjunction`/`negation` 分别对应什么逻辑运算？
5. C++17 的 `_v` 变量模板相比 `::value` 有什么好处？
