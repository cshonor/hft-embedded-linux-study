# 第 3 章 概念、要求与约束

**Concepts, Requirements, and Constraints**

## 本章讲什么

C++20 的 **Concepts（概念）** 是二十年来最受期待的 C++ 特性——给模板参数加**语义约束**，编译错误从天书变成人话。本章是 Concepts 入门：定义、使用、约束方式。

## 要点

### 为什么需要 Concepts

C++17 模板的错误信息是灾难：
```cpp
template <typename T>
void foo(T x) { x.bar(); }

foo(42);  // 编译错误几百行，告诉你 int 没有 bar()，但淹没在模板实例化栈里
```

C++20 Concepts 让约束显式化：
```cpp
template <typename T>
concept HasBar = requires(T x) { x.bar(); };

void foo(HasBar auto x) { x.bar(); }

foo(42);  // 错误：42 不满足 HasBar 约束（int 没有 bar()），一行说清楚
```

### 定义 Concept

```cpp
// 1. 简单表达式
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

// 2. requires 表达式
template <typename T>
concept Addable = requires(T a, T b) {
    a + b;           // 要求 a + b 合法
    { a + b } -> std::convertible_to<T>;  // 要求结果可转 T
};

// 3. 多要求
template <typename T>
concept Comparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
    { a == b } -> std::convertible_to<bool>;
};

// 4. 组合概念
template <typename T>
concept NumericComparable = Numeric<T> && Comparable<T>;
```

### 使用 Concept

四种写法：

```cpp
// 1. template requires
template <typename T> requires Numeric<T>
void foo(T x);

// 2. Concept 直接作约束
template <Numeric T>
void foo(T x);

// 3. auto 简写（推荐）
void foo(Numeric auto x);

// 4. 多参数
template <typename T, typename U>
requires Addable<T> && Addable<U>
auto add(T a, U b) { return a + b; }
```

### `requires` 子句 vs `requires` 表达式

```cpp
// requires 子句：在模板声明里加约束
template <typename T>
requires std::integral<T>     // 这是子句
void foo(T);

// requires 表达式：在 concept 定义里检测
template <typename T>
concept HasFoo = requires(T x) {   // 这是表达式
    x.foo();
};
```

### 约束的偏序

多个重载满足约束时，编译器选**最约束**的：
```cpp
template <typename T> void foo(T);        // 通用
template <typename T> requires std::integral<T>
void foo(T);                              // 整数特化（更约束）

foo(42);   // 选第二个
foo(3.14); // 选第一个
```

## HFT 关联

- **策略接口约束**：`template <typename S> concept Strategy = requires(S s, Tick t) { s.on_tick(t); };` 编译期保证策略类有 `on_tick`。
- **数值类型约束**：`template <Numeric T>` 替代 `enable_if<is_arithmetic>`，错误信息清晰。
- **Range 约束**：`std::ranges::range auto` 接受任意范围，算法泛化。
- **错误信息可读**：HFT 团队协作时，Concepts 让模板错误从"几百行实例化栈"变成"X 不满足 Y 约束"，大幅降低调试成本。
- **重载分派**：`requires` 子句替代 SFINAE/`if constexpr` 做重载分派，编译期选择无运行开销。
- **约束不增加开销**：Concept 是编译期约束，运行期零开销。

## 自测题

1. Concepts 解决了 C++ 模板的什么问题？
2. 定义 Concept 的四种 requires 形式分别是什么？
3. 使用 Concept 的四种写法？哪种推荐？
4. `requires` 子句和 `requires` 表达式的区别？
5. HFT 策略接口如何用 Concept 约束？相比 SFINAE 的好处？
