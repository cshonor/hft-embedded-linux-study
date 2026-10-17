# 第 7 章 仿函数

**Functors**

## 本章讲什么

仿函数是重载 `operator()` 的类。STL 预置了算术（`plus`/`minus`/`multiplies`）、关系（`less`/`greater`/`equal_to`）、逻辑（`logical_and`/`logical_or`）三大类仿函数，并要求它们继承 `unary_function`/`binary_function` 基类以支持适配器配对。本章讲仿函数的源码结构与可配对机制。

## 要点

### 两大基类

```cpp
template<class Arg, class Result> struct unary_function { typedef Arg argument_type; typedef Result result_type; };
template<class Arg1, class Arg2, class Result> struct binary_function { typedef Arg1 first_argument_type; typedef Arg2 second_argument_type; typedef Result result_type; };
```
继承基类 = 内嵌关联类型 = 可被适配器（`not1`/`bind2nd`）配对。这是 SGI 让仿函数"可组合"的关键。

### 三大类预置仿函数

| 类别 | 示例 | 用途 |
|------|------|------|
| 算术 | `plus<T>`/`multiplies<T>` | `accumulate` 求积 |
| 关系 | `less<T>`/`greater<T>` | `sort`/`priority_queue` 比较 |
| 逻辑 | `logical_and<T>` | 谓词组合 |

### 可配对（composable）

继承基类的仿函数能被 `bind1st`/`bind2nd`/`not1`/`not2` 包装成新仿函数。C++11 后 `std::bind` + lambda 基本取代了这套机制。

## HFT 关联

C++11 后手写仿函数基本被 lambda 取代，但理解基类 + 关联类型的设计，能帮你读懂 STL 源码与老代码。HFT 新代码一律用 lambda（可内联、类型唯一）。

## 自测题

1. 仿函数为什么要继承 `unary_function`/`binary_function`？
2. 三大类预置仿函数是什么？各举一例。
3. C++11 后什么取代了 `bind2nd`/`not1` 这套适配器配对？
4. 为什么 HFT 新代码用 lambda 而非手写仿函数？

## 代码自测

### Q1: 仿函数可组合性
```cpp
// STL 内置仿函数组合
std::not1(std::bind2nd(std::less<int>(), 5))  // C++03: !(x < 5) 即 x >= 5

// 现代 C++ 等价
auto pred = [](int x) { return x >= 5; };
auto neg_pred = std::not_fn(pred);  // C++17: !(x >= 5) 即 x < 5
```
> 仿函数的"可组合"意味着什么？为什么 STL 强调函数对象而非函数指针？

<details>
<summary>答案与复习指引</summary>

**可组合**：仿函数可以作为参数传给其他仿函数，形成复合操作。如 `not1(bind2nd(less<int>(), 5))` = `!(x < 5)` = `x >= 5`。

**STL 偏好函数对象的原因**：
1. **可内联**：编译器知道具体类型，可内联 `operator()`
2. **携带状态**：函数对象可存数据（如 `bind` 捕获的参数）
3. **可组合**：通过 `compose`/`bind`/`not_fn` 组合
4. **类型安全**：不同仿函数是不同类型，编译器可据此分派

**C++11 后**：lambda 大幅简化了仿函数使用，但底层仍是闭包类型（编译器生成的函数对象）。`std::function` 是类型擦除的函数包装器，但**有运行时开销**（间接调用 + 可能堆分配），HFT 热路径避免。

**复习：** → [仿函数设计](./README.md)
</details>
