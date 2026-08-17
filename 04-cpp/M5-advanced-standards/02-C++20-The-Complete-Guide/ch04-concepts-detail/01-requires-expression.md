# requires 表达式详解

## 四种要求类型

```cpp
template <typename T>
concept Example = requires(T a, T b, int n) {
    // 1. 简单要求：表达式合法即可
    a.foo();
    a + b;
    a[n];

    // 2. 返回类型要求：{ expr } -> Concept
    { a.bar() } -> std::convertible_to<int>;
    { a < b } -> std::same_as<bool>;

    // 3. 类型要求：嵌套类型必须存在
    typename T::value_type;
    typename T::iterator;
    typename std::iterator_traits<T>::value_type;

    // 4. 嵌套 requires：编译期条件
    requires std::integral<T>;
    requires sizeof(T) >= 4;
    requires std::is_trivially_copyable_v<T>;
};
```

## 简单要求详解

```cpp
template <typename T>
concept Swappable = requires(T a, T b) {
    a.swap(b);  // 成员函数
    swap(a, b); // 自由函数（ADL）
};

// 只检查语法合法性，不检查返回值
// 表达式合法 → 满足；不合法 → 不满足
```

## 返回类型要求

```cpp
template <typename T>
concept Hashable = requires(T a) {
    // 返回类型必须可转换为 std::size_t
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept Comparable = requires(T a, T b) {
    // 返回类型必须精确是 bool
    { a < b } -> std::same_as<bool>;
    // 可转换到 bool（允许 int、bool 等）
    { a == b } -> std::convertible_to<bool>;
};
```

**区别**：
- `-> std::same_as<bool>`：返回类型必须精确是 `bool`
- `-> std::convertible_to<bool>`：返回类型能隐式转 `bool`（int 也行）

## 类型要求

```cpp
template <typename T>
concept HasValueType = requires {
    typename T::value_type;  // T 必须有 value_type 嵌套类型
};

template <typename T>
concept HasIterTraits = requires {
    typename std::iterator_traits<T>::value_type;  // 实例化 iterator_traits<T>
};
```

## 嵌套 requires

```cpp
template <typename T>
concept SafeNumeric = requires {
    requires std::is_arithmetic_v<T>;  // 编译期常量
    requires sizeof(T) <= 8;           // 大小限制
};

// 嵌套 requires 和外层 requires 的区别：
template <typename T>
concept C1 = requires(T t) {
    requires std::integral<T>;  // 嵌套：作为 requires 表达式的一部分
};
template <typename T>
concept C2 = std::integral<T>;  // 直接：Concept 组合
// C1 和 C2 效果类似，但 C1 可以和其他要求混在同一个 requires 中
```

## requires 子句 vs requires 表达式

```cpp
// requires 子句：约束模板
template <typename T>
requires std::integral<T>  // requires 子句
void foo(T x) { }

// requires 表达式：定义 Concept
template <typename T>
concept HasFoo = requires(T x) {  // requires 表达式
    x.foo();
};

// 两者可以组合
template <typename T>
requires requires(T x) { x.foo(); }  // requires 子句 + requires 表达式
void bar(T x) { x.foo(); }
// 但通常用 Concept 代替
```

## 自测题

1. requires 表达式的四种要求分别是什么？
2. `-> std::same_as<bool>` 和 `-> std::convertible_to<bool>` 的区别？
3. 类型要求 `typename T::value_type` 检测什么？
4. 嵌套 requires 和直接 Concept 组合有什么区别？
5. requires 子句和 requires 表达式的区别？能组合使用吗？
