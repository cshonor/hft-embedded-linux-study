# 第 4 章 深入理解概念、要求与约束

**Concepts, Requirements, and Constraints in Detail**

## 本章讲什么

Concepts 的进阶：requires 表达式的细节、原子约束与合取/析取、约束的偏序规则、子sumeption（包含关系）、 Concept 与 auto 的推导。

## 要点

### requires 表达式的四种要求

```cpp
template <typename T>
concept Example = requires(T a, T b, int n) {
    // 1. 简单要求：表达式合法
    a.foo();
    a + b;

    // 2. 返回类型要求：表达式合法且返回可转某类型
    { a.bar() } -> std::convertible_to<int>;
    { a < b } -> std::same_as<bool>;

    // 3. 类型要求：嵌套类型存在
    typename T::value_type;
    typename std::iterator_traits<T>::value_type;

    // 4. 复合要求：嵌套 requires
    requires std::integral<T>;
    requires sizeof(T) >= 4;
};
```

### 原子约束与逻辑组合

```cpp
template <typename T>
concept A = std::integral<T>;
template <typename T>
concept B = std::floating_point<T>;
template <typename T>
concept C = A<T> || B<T>;        // 析取（||）
template <typename T>
concept D = A<T> && !B<T>;       // 合取（&&）+ 否定
```

Concept 间的逻辑组合用 `&&`/`||`/`!`，编译器解析为原子约束的合取/析取。

### subsumption（约束包含）

如果一个约束 A **蕴含** 约束 B（A 更严格），则 A "包含" B。重载时编译器选更约束的：

```cpp
template <typename T> requires std::integral<T>
void foo(T);                       // 通用整数

template <typename T> requires std::integral<T> && (sizeof(T) == 4)
void foo(T);                       // 4 字节整数（更约束）

foo(42);    // int 是 4 字节 → 选第二个
foo(int64_t{});  // 8 字节 → 选第一个
```

subsumption 要求约束是**相同的原子约束**（不是仅逻辑等价）——用 Concept 名而非内联表达式才能 subsume。

### Concept 与 auto 推导

```cpp
template <typename T>
concept Integral = std::integral<T>;

Integral auto x = 42;     // auto 推导 int，Concept 检查通过
// Integral auto y = 3.14; // 错误：double 不满足 Integral
```

`Concept auto` 形式让变量声明也带约束。

### Concept 的友元与重载

```cpp
// 模板类的 Concept 重载
template <typename T>
struct Wrapper {
    void process() requires std::integral<T> { /* int 版 */ }
    void process() requires std::floating_point<T> { /* float 版 */ }
};
```

成员函数也可以带 requires，编译期分派。

## HFT 关联

- **细粒度策略重载**：`on_tick` 对 L1/L2 行情用 `requires` 分派，编译期选最优实现。
- **返回类型约束**：`{ strategy.calc() } -> std::same_as<double>` 保证策略返回 double，编译期捕获类型错误。
- **类型要求检测嵌套类型**：`typename T::price_type` 确保策略类定义了 price_type 类型。
- **subsumption 做特化**：通用行情处理 + 特定合约类型特化，用 subsumption 让编译器选特化版。
- **Concept 变量声明**：`Integral auto qty = order.qty` 编译期保证 qty 是整数类型。

## 自测题

1. requires 表达式的四种要求分别是什么？
2. Concept 的合取、析取、否定如何写？
3. subsumption（约束包含）是什么？为什么要求"相同原子约束"？
4. `Concept auto x = ...` 的推导和检查如何工作？
5. HFT 如何用 subsumption 做行情处理特化？
