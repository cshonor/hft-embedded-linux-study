# 第 5 章 深入理解标准概念

**Standard Concepts in Detail**

## 本章讲什么

C++20 `<concepts>` 头提供一批预置 Concept。本章详述这些标准 Concept 的分类和用法，避免重复造轮子。

## 要点

### 标准 Concept 分类

| 类别 | 代表 Concept |
|------|--------------|
| 语言相关 | `same_as`、`derived_from`、`convertible_to`、`common_reference_with` |
| 算术 | `integral`、`floating_point`、`arithmetic`、`signed_integral`、`unsigned_integral` |
| 比较 | `equality_comparable`、`totally_ordered`、`three_way_comparable` |
| 对象语义 | `movable`、`copyable`、`default_initializable`、`semiregular`、`regular` |
| 可调用 | `invocable`、`regular_invocable`、`predicate` |
| 范围/迭代器 | `ranges::range`、`input_iterator`、`random_access_iterator` 等（见第 6-8 章） |

### 常用 Concept 详解

```cpp
// 类型关系
template <typename T>
concept SameAsInt = std::same_as<T, int>;

template <typename Derived, typename Base>
concept IsDerived = std::derived_from<Derived, Base>;

// 算术
template <typename T> requires std::integral<T>      // 整数
template <typename T> requires std::floating_point<T> // 浮点
template <typename T> requires std::arithmetic<T>     // 算术（整数或浮点）

// 比较
std::equality_comparable<T>     // 有 == 和 !=
std::totally_ordered<T>         // 有 <, >, <=, >= 且全序

// 对象语义
std::movable<T>                 // 可移动
std::copyable<T>                // 可拷贝
std::semiregular<T>             // copyable + default_initializable（像 int）
std::regular<T>                 // semiregular + equality_comparable（像 int 有 ==）

// 可调用
std::invocable<F, Args...>      // F 可用 Args 调用
std::regular_invocable<F, Args...>  // 同上且调用无副作用（纯函数）
std::predicate<F, Args...>      // 可调用且返回 bool
```

### `regular` 与 `semiregular`

- `semiregular`：可默认构造、可拷贝、可移动——"像 int 一样能存进容器"。
- `regular`：semiregular + 可相等比较——"像 int 一样正常"。

```cpp
template <typename T>
requires std::regular<T>
void foo(T x);   // T 像 int 一样正常
```

### 使用建议

| 场景 | 推荐 Concept |
|------|--------------|
| 数值算法 | `std::arithmetic` 或 `std::integral` |
| 容器元素 | `std::regular` |
| 回调 | `std::invocable` 或 `std::predicate` |
| 基类约束 | `std::derived_from` |
| 类型一致 | `std::same_as` |
| 排序 | `std::totally_ordered` |

### 不要过度约束

```cpp
// 过度约束：只要能比较就够，不需要 totally_ordered
template <std::totally_ordered T>  // 太严
void find_min(const std::vector<T>& v);

// 合适：只要能 <
template <typename T> requires requires(T a, T b) { a < b; }
void find_min(const std::vector<T>& v);
```

约束应**恰好满足算法需要**，过严会拒绝合法类型。

## HFT 关联

- **数值约束用 `arithmetic`/`integral`**：策略参数模板用 `arithmetic auto` 约束，替代手写 `is_arithmetic`。
- **`regular` 约束数据类型**：Tick/Order 这类值类型要求 `regular`——可默认构造、可拷贝、可比较。
- **`predicate` 约束过滤函数**：`template <std::predicate<Tick> F>` 约束过滤回调返回 bool。
- **`derived_from` 约束策略基类**：策略模板 `requires std::derived_from<S, StrategyBase>` 确保继承层次。
- **`totally_ordered` 约束排序键**：订单按价格排序，`totally_ordered` 保证价格全序可比。
- **避免过度约束**：只用算法真正需要的最小 Concept，保持泛型灵活性。

## 自测题

1. `regular` 和 `semiregular` 的区别？为什么 `regular` 更"像 int"？
2. `invocable`、`regular_invocable`、`predicate` 的区别？
3. 为什么 Concept 约束要"恰好满足算法需要"，不要过度约束？
4. HFT 数值算法应该用哪个标准 Concept？策略回调呢？
5. `totally_ordered` 和 `equality_comparable` 的区别？

## 代码自测

### Q1: 标准库概念
```cpp
#include <concepts>

// 常用标准概念
static_assert(std::integral<int>);           // true
static_assert(std::floating_point<double>);  // true
static_assert(std::same_as<int, int>);       // true

// 关系概念
static_assert(std::totally_ordered<int>);    // int 支持所有比较

// 对象概念
static_assert(std::movable<std::string>);    // string 可移动
static_assert(std::copyable<std::string>);   // string 可拷贝
static_assert(std::regular<int>);            // int 是 regular（可默认构造+拷贝+比较）

// 可调用概念
static_assert(std::invocable<decltype({}), int>);  // lambda(int) 可调用
```
> `regular` 概念包含哪些要求？为什么重要？

<details>
<summary>答案与复习指引</summary>

**`regular`** = `semiregular` + `equality_comparable`：
- `semiregular` = `copyable` + `default_constructible`（可拷贝、可默认构造）
- `equality_comparable` = 支持 `==`/`!=`

即：regular 类型可以默认构造、拷贝、赋值、比较相等——像 `int` 一样"普通"。

**为什么重要**：
- regular 类型可以存在容器中、可以作为值传递、可以比较——满足大部分通用算法的要求
- STL 算法隐式假设元素是 regular 的
- 值语义编程的核心：自定义类型尽量满足 `regular`

**标准概念分类**：
| 类别 | 示例 |
|------|------|
| 语言相关 | `integral`/`floating_point`/`signed_integral` |
| 关系 | `same_as`/`derived_from`/`convertible_to`/`common_with` |
| 对象 | `movable`/`copyable`/`semiregular`/`regular` |
| 可调用 | `invocable`/`predicate`/`strict_weak_order` |
| 范围 | `range`/`input_range`/`random_access_range` |

**复习：** → [标准概念](./README.md)
</details>
