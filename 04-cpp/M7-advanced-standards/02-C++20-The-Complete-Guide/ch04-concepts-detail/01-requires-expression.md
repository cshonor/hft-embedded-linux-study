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

<details>
<summary>参考答案</summary>

1. requires 表达式内部可以写四类要求：
   1. **简单要求**：`x.foo();` —— 只检查表达式是否**合法**（不检查结果类型）。
   2. **类型要求**：`typename T::value_type;` —— 检查某个**嵌套类型**是否存在。
   3. **复合要求**：`{ expr } -> Concept;`（还可写 `{ expr } noexcept -> Concept;`）—— 检查表达式合法**且结果类型满足 Concept**，以及（可选）是否 `noexcept`。
   4. **嵌套要求**：`requires ConstantExpression;` —— 检查一个**编译期布尔条件**成立。
2. 两者都写在复合要求的 `->` 后面，但严格程度不同：
   - `{ expr } -> std::same_as<bool>`：要求 `decltype((expr))` **恰好是** `bool`——返回 `int`、`const bool&`、可转 bool 的类类型都**不**满足。
   - `{ expr } -> std::convertible_to<bool>`：只要求返回类型**能转换**成 bool——`int`、`bool`、带 `operator bool` 的类型都可以。
写接口约束时一般用 `convertible_to<bool>`（宽松、符合"布尔可测试"语义）；只有当返回类型必须精确匹配时才用 `same_as`。
3. 它检测 `T` 内部**是否存在名为 `value_type` 的成员类型**（嵌套类型/类型别名），属于"类型要求"。
```cpp
template <typename T>
concept HasValueType = requires { typename T::value_type; };
```
它比"检查 `iterator_traits<T>::value_type`"更直接：后者还会**实例化** `std::iterator_traits<T>`（对某些 T 可能是硬错误或不完整类型），这也是为什么标准库里会区分 `has-typename-value_type`（直接查嵌套类型）和 `HasIterTraits`（查 traits）两种写法。
4. 两者都能表达"要求某个编译期条件成立"，但位置与后果不同：
   - **嵌套 requires** 写在 requires 表达式**内部**（`concept C1 = requires(T t) { requires std::integral<T>; };`），好处是可以和简单要求、类型要求、复合要求**混在同一个 requires 表达式里**。
   - **直接组合**是 concept 定义层面的合取（`concept C2 = std::integral<T>;`）。
关键差别在 **subsumption**：requires 表达式整体是一个**原子约束**，subsumption 分析不会把它内部拆开，所以 `C1` 并不 subsumes `std::integral<T>`；而 `C2` 会。想让更严格的 concept 在重载中胜出，就应该用直接组合（引用同一个 concept）。
5. - **requires 子句**（requires-clause）：写在模板/函数**声明**上的约束载体，`template<typename T> requires std::integral<T> void foo(T);` 里那个 `requires` 是子句。
   - **requires 表达式**（requires-expression）：一个**表达式**，求值为 bool，用来描述"这些表达式是否合法"，通常用来定义 concept 或就地检查。
两者可以组合，于是出现了 `requires requires`：
```cpp
template <typename T>
requires requires(T x) { x.foo(); }   // 外层：子句；内层：表达式
void bar(T x) { x.foo(); }
```
实践中通常把内层抽成一个命名的 concept（`requires HasFoo<T>`），既可读又能参与 subsumption。

</details>
