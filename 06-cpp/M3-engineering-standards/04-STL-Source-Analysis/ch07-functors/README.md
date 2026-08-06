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
