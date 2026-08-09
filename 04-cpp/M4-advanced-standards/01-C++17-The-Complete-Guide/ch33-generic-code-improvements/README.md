# 第 33 章 泛型代码实现改进

**Improvements for Implementing Generic Code**

## 本章讲什么

C++17 的一些让泛型代码实现更简洁、更正确的特性聚合：`if constexpr`（第 10 章）、折叠表达式（第 11 章）、`void_t`、`is_invocable`、变量模板 `_v` 等。本章是"如何用这些工具写出更好的泛型库"的总结。

## 要点

### 用 `if constexpr` 替代 SFINAE 分派

```cpp
// C++14 SFINAE：冗长
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
void process(T x) { /* int 版 */ }
template <typename T,
          std::enable_if_t<!std::is_integral_v<T>, int> = 0>
void process(T x) { /* other 版 */ }

// C++17：if constexpr
template <typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) {
        /* int 版 */
    } else {
        /* other 版 */
    }
}
```

### `void_t` 的 SFINAE 检测

```cpp
// 检测类型是否有成员函数 foo()
template <typename, typename = std::void_t<>>
struct HasFoo : std::false_type {};

template <typename T>
struct HasFoo<T, std::void_t<decltype(std::declval<T>().foo())>> : std::true_type {};

template <typename T>
constexpr bool has_foo_v = HasFoo<T>::value;

// 使用
if constexpr (has_foo_v<T>) { x.foo(); }
```

`void_t` 让"检测成员是否存在"的 SFINAE 极简洁。

### 折叠表达式做编译期校验

```cpp
// 所有类型必须是算术类型
template <typename... Ts>
constexpr bool all_arithmetic = (std::is_arithmetic_v<Ts> && ...);

template <typename... Ts>
void compute(Ts... ts) {
    static_assert(all_arithmetic<Ts...>, "all types must be arithmetic");
    // ...
}
```

### `invoke_result` 推导返回类型

```cpp
// C++14 result_of 有已知问题，C++17 invoke_result 替代
template <typename F, typename... Args>
auto wrap(F&& f, Args&&... args) -> std::invoke_result_t<F, Args...> {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}
```

`std::invoke` 统一了普通调用和成员指针调用。

### `std::invoke` 统一调用

```cpp
struct Obj { int foo(int) { return 1; } };

Obj o;
std::invoke(&Obj::foo, o, 42);      // 成员函数指针
std::invoke([](int x){ return x; }, 42);  // lambda

// invoke_result 配合 invoke
using R = std::invoke_result_t<decltype(&Obj::foo), Obj, int>;  // int
```

### 泛型 lambda + if constexpr

```cpp
auto visitor = [](auto&& x) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::is_integral_v<T>) {
        std::cout << "int: " << x;
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "str: " << x;
    }
};
```

## HFT 关联

- **`if constexpr` 简化策略分派**：对不同行情类型（Tick/Trade/Book）用 `if constexpr` 分派，无 SFINAE 噪音。
- **`void_t` 检测能力**：检测策略类是否有特定方法（`on_tick`/`on_trade`），有则调用、无则跳过。
- **折叠表达式批量约束**：`static_assert((is_trivially_copyable_v<Ts> && ...))` 保证所有模板参数可 memcpy。
- **`invoke` 统一回调**：策略回调可能是函数对象、lambda、成员函数指针，`invoke` 统一调用。
- **泛型 lambda + visit**：variant 消息处理用泛型 lambda + `if constexpr`，编译期分派无虚函数开销。

## 自测题

1. `if constexpr` 相比 SFINAE 在泛型分派上的好处是什么？
2. `void_t` 如何检测类型是否有某成员？写一个检测 `foo()` 的例子。
3. 折叠表达式如何做编译期批量类型约束？
4. `std::invoke` 统一了什么？为什么比直接调用好？
5. HFT variant 消息处理如何用泛型 lambda + `if constexpr`？

## 代码自测

### Q1: 泛型代码改进
```cpp
// C++17: noexcept 作为类型的一部分
void (*fp1)() noexcept = []() noexcept {};  // OK
// void (*fp2)() = fp1;  // C++17 前可以，C++17 后严格了

// constexpr if 简化 SFINAE
template<typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integer: " << x;
    } else {
        std::cout << "other: " << x;
    }
}

// auto 非类型模板参数
template<auto N> void print() { std::cout << N; }
```
> C++17 对泛型编程有哪些改进？

<details>
<summary>答案与复习指引</summary>

C++17 泛型改进：
1. **constexpr if**：替代 SFINAE/enable_if 做编译期分支
2. **fold expressions**：简化可变参数模板展开
3. **auto NTTP**：简化非类型模板参数
4. **if-init in constexpr**：constexpr 函数中可用 if-init
5. **`noexcept` 类型化**：函数指针的 noexcept 是类型的一部分
6. **CTAD**：类模板参数自动推导
7. **inline variables**：头文件中定义变量模板

这些改进让泛型代码更简洁、更易读、更少模板元编程技巧。

**复习：** → [泛型代码改进](./README.md)
</details>
