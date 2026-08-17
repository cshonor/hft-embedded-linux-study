# 第 13 章 auto 作模板参数

**Placeholder Types like auto as Template Parameters**

## 本章讲什么

C++17 允许模板参数写 `auto` 代替 `typename`/具体类型，编译器从实参推导。这让"任意非类型参数"的模板写法大幅简化。

## 要点

### 基本用法

```cpp
// C++14：要写具体类型
template <typename T, T N>
struct Constant { static constexpr T value = N; };
Constant<int, 42> c;   // 要写 int

// C++17：auto 推导
template <auto N>
struct Constant { static constexpr decltype(N) value = N; };
Constant<42> c;        // N 推导为 int
Constant<'x'> c2;      // N 推导为 char
Constant<3.14> c3;     // N 推导为 double（C++20 才支持 double NTTP，C++17 仅整型/指针/引用）
```

### `template<auto>` 与函数

```cpp
template <auto N>
void foo() {
    std::cout << N;
}
foo<42>();     // int
foo<"hi">();   // C++17 仍不支持字符串字面量
```

### `decltype(auto)` 作模板参数

```cpp
template <decltype(auto) N>  // 保留引用性
struct Ref { };
int x = 0;
Ref<x> r;   // N 是 int&（decltype(auto) 保留引用）
```

### 用途

```cpp
// 1. 编译期常量容器
template <auto... Values>
struct ValueList {};
ValueList<1, 2.0, 'x'> vl;   // 异质值列表

// 2. 标签类型
template <auto Tag>
struct Tagged {};
Tagged<42> t;

// 3. 函数指针作模板参数
template <auto Func>
struct Caller {
    void call() { Func(); }
};
Caller<&some_function> c;
```

### 限制

- C++17 的 NTTP 只支持**整型、枚举、指针、引用、字面量类型**（如 FixedString）。浮点 C++20 才支持。
- `auto` 推导遵循 `auto` 的规则（去掉引用、顶层 const），`decltype(auto)` 保留。

## HFT 关联

- **编译期配置值**：`Config<BUF_SIZE>` 用 `template<auto N>` 让任意类型常量（int、enum、函数指针）都能编译期传入。
- **函数指针标签**：`Dispatcher<&handler>` 把回调函数指针作模板参数，编译期绑定无虚函数开销。
- **枚举标签**：`Route<Channel::FEED>` 用枚举值作模板参数，类型安全。
- **异质值列表**：`ValueList<1, 2.0, 'x'>` 编译期聚合不同类型值，元编程用。
- **C++17 的限制**：浮点 NTTP 要 C++20，HFT 策略参数若用 double 做 NTTP 需升级。

## 自测题

1. `template <auto N>` 相比 `template <typename T, T N>` 有什么好处？
2. `template <auto>` 和 `template <decltype(auto)>` 的区别是什么？
3. C++17 的 NTTP 支持哪些类型？不支持哪些？
4. 函数指针作模板参数有什么用？HFT 怎么用？
5. C++20 对 NTTP 的扩展是什么（浮点）？

## 代码自测

### Q1: auto 模板参数
```cpp
// C++17: auto 作为非类型模板参数
template<auto N>
struct Constant {
    static constexpr auto value = N;
};

Constant<42> c1;       // N = int 42
Constant<'a'> c2;      // N = char 'a'
Constant<3.14> c3;     // C++20 才支持 double，C++17 仅支持整型/指针/引用

// 函数模板也支持
template<auto... Vs>
void print_all() { (std::cout << ... << Vs) << '\n'; }
print_all<1, 'x', 3L>();  // 输出 1x3
```
> C++17 的 `auto` 模板参数相比之前的 `template<typename T, T N>` 有什么好处？

<details>
<summary>答案与复习指引</summary>

**好处**：不需要先写类型参数 `typename T` 再用 `T N`，`auto N` 一步到位，编译器自动推导类型。

```cpp
// C++14: 两步
template<typename T, T N> struct Constant14 {};
Constant14<int, 42> c;  // 需要显式写 int

// C++17: 一步
template<auto N> struct Constant17 {};
Constant17<42> c;  // 自动推导 int
```

**限制**（C++17）：N 的类型必须是整型、枚举、指针、引用或 `nullptr_t`。浮点数/字符串/对象不能做 NTTP（C++20 部分放宽）。

**复习：** → [auto 模板参数](./README.md)
</details>
