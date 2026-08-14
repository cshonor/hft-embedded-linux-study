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
