# 定义 Concept

## 基本语法

```cpp
#include <concepts>

// 1. 基于 traits
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

// 2. 基于 requires 表达式
template <typename T>
concept Addable = requires(T a, T b) {
    a + b;
};

// 3. 多重要求
template <typename T>
concept Container = requires(T c) {
    c.begin();
    c.end();
    c.size();
    typename T::value_type;
};

// 4. 组合现有 Concept
template <typename T>
concept NumericContainer = Container<T> && Numeric<typename T::value_type>;
```

## requires 表达式

```cpp
template <typename T>
concept Drawable = requires(const T& obj, std::ostream& os) {
    { obj.draw(os) } -> std::same_as<void>;
    { obj.area() } -> std::floating_point<double>;  // 返回浮点
};

// 四种 requires：
// 1. 简单表达式：a + b（合法即可）
// 2. 返回类型：{ expr } -> Concept
// 3. 类型要求：typename T::value_type
// 4. 嵌套约束：requires Concept<T>
```

## 带参数的 requires

```cpp
template <typename T>
concept Stack = requires(T s, typename T::value_type v) {
    s.push(v);      // push 方法
    s.pop();        // pop 方法
    { s.top() } -> std::same_as<typename T::value_type&>;  // top 返回引用
    { s.empty() } -> std::convertible_to<bool>;  // empty 返回 bool
};
```

## Concept 组合

```cpp
// 合取（AND）
template <typename T>
concept A = std::integral<T>;
template <typename T>
concept B = std::signed_integral<T>;

template <typename T>
concept C = A<T> && B<T>;  // 既是整数又是带符号

// 析取（OR）
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// 原子约束不可拆分
// template <typename T>
// concept D = A<T> || !A<T>;  // 总是 true（但不是恒真，因为析取有特殊规则）
```

## 自测题

1. 定义 Concept 的基本语法是什么？
2. `requires` 表达式有哪四种要求？
3. 如何约束返回类型？`{ expr } -> Concept` 的含义？
4. Concept 如何组合（合取/析取）？
5. 定义一个 `Stack` concept，要求有 push/pop/top/empty 方法。

<details>
<summary>参考答案</summary>

1. 基本语法是把一个**约束表达式**赋给 concept：
```cpp
template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>            // 带多个参数也可以
concept SameAs = std::is_same_v<T, U>;
```
约束表达式必须是编译期 `bool`（可以是 concept、traits 的 `_v`、常量表达式，或者 requires 表达式）。
2. requires 表达式内的四类要求：
   1. **简单要求**（simple requirement）：`x.foo();` —— 只要求表达式**合法**。
   2. **类型要求**（type requirement）：`typename T::value_type;` —— 要求某个**嵌套类型**存在。
   3. **复合要求**（compound requirement）：`{ x.foo() } -> std::same_as<int>;` —— 要求表达式合法**且返回类型满足某个 concept**（可再加 `noexcept`）。
   4. **嵌套要求**（nested requirement）：`requires std::integral<T>;` —— 要求一个**编译期布尔条件**成立。
3. 复合要求的 `->` 用来约束**表达式的返回类型**：`{ expr } -> Concept` 表示"`decltype((expr))` 必须满足 Concept"。
```cpp
template <typename S>
concept Strategy = requires(S s, const Tick& t) {
    { s.on_tick(t) }     -> std::same_as<void>;
    { s.should_trade() } -> std::convertible_to<bool>;
};
```
注意 `-> std::same_as<bool>` 要求**恰好**是 `bool`；`-> std::convertible_to<bool>` 只要求**能转换**成 bool（更宽松）。
4. 用逻辑运算符直接组合（合取 `&&`、析取 `||`），并且组合的是 concept 而不是裸表达式：
```cpp
template <typename T>
concept SignedNumeric = std::integral<T> && std::signed_integral<T>;

template <typename T>
concept NumberOrString = std::integral<T> || std::floating_point<T>
                      || std::same_as<T, std::string>;
```
要点：concept 之间**不能**重载或特化；组合时优先用"已有 concept 的 `&&`/`||`"而不是把条件写进一个大的 requires 表达式——后者会影响 subsumption（见第 4 章）。
5. ```cpp
template <typename S>
concept Stack = requires(S s) {
    { s.empty() }  -> std::convertible_to<bool>;
    { s.top()   };                       // 至少有 top()，返回类型不约束
    s.push(std::declval<typename S::value_type>());
    s.pop();
    typename S::value_type;              // 要求有 value_type
};
```
说明：`empty()` 要求返回可转 bool；`push` 用 `declval<value_type>()` 造一个元素；`top()` 用简单要求表示"能调用即可"（也可以加 `->` 约束返回 `value_type&`）；`typename S::value_type;` 是类型要求。

</details>
