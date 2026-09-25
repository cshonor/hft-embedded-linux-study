# void_t SFINAE 检测

## 检测成员是否存在

```cpp
#include <type_traits>

// 检测 T 是否有 value_type
template <typename, typename = std::void_t<>>
struct HasValueType : std::false_type {};

template <typename T>
struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type {};

template <typename T>
constexpr bool has_value_type_v = HasValueType<T>::value;

static_assert(has_value_type_v<std::vector<int>>);  // true
static_assert(!has_value_type_v<int>);               // false
```

## SFINAE 原理

```
HasValueType<int, void_t<>>:
  → void_t<> = void（空参数包，合法）
  → 继承 false_type

HasValueType<vector<int>, void_t<vector<int>::value_type>>:
  → vector<int>::value_type = int（合法）
  → void_t<int> = void
  → 匹配特化版本，继承 true_type

HasValueType<int, void_t<int::value_type>>:
  → int::value_type 不存在 → 替换失败（SFINAE）
  → 回退到主模板，继承 false_type
```

## 检测成员函数

```cpp
// 检测 T 是否有 foo() 方法
template <typename, typename = std::void_t<>>
struct HasFoo : std::false_type {};

template <typename T>
struct HasFoo<T, std::void_t<decltype(std::declval<T>().foo())>>
    : std::true_type {};

template <typename T>
constexpr bool has_foo_v = HasFoo<T>::value;

struct A { void foo() {} };
struct B {};

static_assert(has_foo_v<A>);   // true
static_assert(!has_foo_v<B>);  // false
```

## 结合 if constexpr

```cpp
template <typename T>
void process(T& x) {
    if constexpr (has_foo_v<T>) {
        x.foo();  // 有 foo() 就调用
    } else {
        // 没 foo() 走通用路径
        std::cout << "no foo";
    }
}
```

## 检测多个成员

```cpp
// 检测 T 是否同时有 begin() 和 end()
template <typename, typename = void>
struct IsContainer : std::false_type {};

template <typename T>
struct IsContainer<T, std::void_t<
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end())
>> : std::true_type {};

template <typename T>
constexpr bool is_container_v = IsContainer<T>::value;

static_assert(is_container_v<std::vector<int>>);  // true
static_assert(!is_container_v<int>);               // false
```

## 自测题

1. `void_t` 的 SFINAE 原理是什么？
2. 如何检测类型是否有某个成员函数？
3. 如何检测类型是否同时有 `begin()` 和 `end()`？
4. `void_t` 在 C++14 和 C++17 中的地位有什么区别？
5. `void_t` + `if constexpr` 如何实现"有则调用、无则跳过"？

<details>
<summary>参考答案</summary>

1. `std::void_t<Args...>` 是别名模板，展开结果恒为 `void`；关键在它**在模板实参替换的语境中**展开。
把它放在偏特化的模板实参列表里（如 `HasFoo<T, std::void_t<decltype(...)>>`）：若 `decltype(...)` 不合法，替换失败属于 **SFINAE**（软错误），该偏特化被静默丢弃，匹配回主模板（`false_type`）；若合法则偏特化更特化而被选中（`true_type`）。于是"成员是否存在"就被转成了"选哪个模板"。
2. 用 `decltype` 写出一次调用，放进 `void_t`，配合偏特化：
```cpp
template <typename, typename = void>
struct HasFoo : std::false_type {};

template <typename T>
struct HasFoo<T, std::void_t<decltype(std::declval<T&>().foo())>> : std::true_type {};

template <typename T> constexpr bool has_foo_v = HasFoo<T>::value;
static_assert(has_foo_v<A>);    // A 有 foo()
static_assert(!has_foo_v<B>);   // B 没有
```
`std::declval<T&>()` 在不构造对象的前提下得到一个 `T&`，`decltype(...foo())` 合法即表示"能以无参方式调用 `foo()`"；要检查签名就写成 `decltype(std::declval<T&>().foo(0))`。
3. 把两个 `decltype` 一起放进 `void_t`，只有两者都合法偏特化才成立：
```cpp
template <typename, typename = void>
struct IsContainer : std::false_type {};

template <typename T>
struct IsContainer<T, std::void_t<
    decltype(std::declval<T&>().begin()),
    decltype(std::declval<T&>().end())
>> : std::true_type {};
```
`void_t` 接受任意多个类型参数，全部替换成功才得到 `void`——所以它是"**同时**满足多个条件"的天然写法（逻辑与）。
4. C++14 里 `void_t` 只是一个**惯用法**：必须写成 `template <class...> using void_t = void;` 并包在一个辅助结构体里（因为 CWG 1558 之前，别名模板中未被使用的实参不保证触发替换失败）。
C++17 把它**标准化**为 `std::void_t`，同时 CWG 1558 已得到解决，语义有明确保证——可以直接用，不再需要那个 `voider` 间接层。
5. 用 `void_t` 把"是否有某成员"变成编译期布尔值，再用 `if constexpr` 在函数体里分派：
```cpp
template <typename T>
void process(T& x) {
    if constexpr (has_foo_v<T>) {
        x.foo();                 // 只有该分支会被实例化
    } else {
        std::cout << "no foo";   // 无 foo() 时走这里
    }
}
```
`has_foo_v<T>` 恒为编译期常量，被丢弃的分支不实例化，因此 `T` 没有 `foo()` 时 `x.foo()` 不会导致编译错误——这正是 C++17 取代「SFINAE 重载 + tag dispatch」的简洁写法（C++20 之后可进一步用 concepts / requires 表达式）。

</details>
