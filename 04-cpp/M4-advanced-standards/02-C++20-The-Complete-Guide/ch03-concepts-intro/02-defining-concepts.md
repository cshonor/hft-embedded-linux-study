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
