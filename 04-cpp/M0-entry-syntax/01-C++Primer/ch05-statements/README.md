# 第 5 章 语句

本章介绍 C++ 的控制流（flow-of-control）语句，支持程序跳出简单顺序执行，实现条件分支、循环和跳转等复杂执行路径。

## 小节

- [5.1–5.2 简单语句与作用域](./5.1-5.2简单语句与作用域.md)
- [5.3 条件语句](./5.3-条件语句.md)
- [5.4 迭代语句（循环）](./5.4-迭代语句（循环）.md)
- [5.5 跳转语句](./5.5-跳转语句.md)
- [5.6 try 语句块和异常处理](./5.6-try语句块和异常处理.md)


## 章节摘要

C++ 的控制流语句：简单语句与复合语句、作用域（`if`/`switch`/`for` 引入块作用域）、条件语句（`if`/`switch`）、迭代语句（`while`/`for`/`do-while`/范围 for）、跳转语句（`break`/`continue`/`goto`/`return`）以及异常处理（`try`/`catch`/`throw`）。

### 和 C 的区别

| C | C++ |
|---|-----|
| 变量声明在块开头 | 可在任意位置声明（C99 也支持） |
| 无范围 for | `for (auto &x : container)` |
| `setjmp`/`longjmp` | `try`/`catch`/`throw` 异常机制 |
| `switch` 不自动 break | 相同（贯穿陷阱） |

## 章节自测

### Q1: switch 贯穿

```cpp
int x = 2;
switch (x) {
    case 1: std::cout << "1 ";
    case 2: std::cout << "2 ";
    case 3: std::cout << "3 ";
    default: std::cout << "d ";
}
```

> 输出是什么？如何避免贯穿？

<details>
<summary>答案与复习指引</summary>

**输出：** `2 3 d `

**原因：** `switch` 匹配到 `case 2` 后，从那里开始执行，没有 `break` 就会"贯穿"执行后续所有 case 的代码，直到 `break` 或 switch 结束。

**避免：** 每个 case 后加 `break`。

**复习：** → [5.3 条件语句](./5.3-条件语句.md)
</details>

### Q2: 范围 for

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (auto x : v)      // A
    x *= 2;
for (auto &x : v)     // B
    x *= 2;
```

> A 和 B 执行后 v 分别是什么？

<details>
<summary>答案与复习指引</summary>

**A 后：** v 不变 `{1,2,3,4,5}`——`auto x` 是值拷贝，修改拷贝不影响原元素
**B 后：** v 变为 `{2,4,6,8,10}`——`auto &x` 是引用，直接修改原元素

**教训：** 范围 for 要修改容器元素必须用引用 `auto &`。只读时用 `const auto &` 避免拷贝。

**复习：** → [5.4 迭代语句（循环）](./5.4-迭代语句（循环）.md)
</details>

### Q3: 异常基础

```cpp
#include <stdexcept>
double divide(int a, int b) {
    if (b == 0)
        throw std::runtime_error("div by zero");
    return static_cast<double>(a) / b;
}
int main() {
    try {
        std::cout << divide(10, 0);
    } catch (const std::runtime_error &e) {
        std::cout << "Error: " << e.what();
    }
}
```

> 输出是什么？`catch` 为什么要用引用？

<details>
<summary>答案与复习指引</summary>

**输出：** `Error: div by zero`

**`catch` 用引用的原因：**
1. 避免拷贝异常对象（可能派生类切片——按值 catch 基类会丢失派生部分）
2. 多态——`catch(const std::exception&)` 能捕获所有派生自 `std::exception` 的异常

**和 C 的区别：** C 用返回值/`errno` 错误处理，或 `setjmp`/`longjmp`（不能析构栈对象）。C++ 异常在栈展开时自动调用析构函数（RAII）。

**复习：** → [5.6 try 语句块和异常处理](./5.6-try语句块和异常处理.md)
</details>

### Q4: goto 与作用域

```cpp
void f() {
    goto label;          // A
    std::string s = "hi";
label:
    std::cout << s;      // B: s 已构造？
}
```

> A 行合法吗？B 行 `s` 的状态是什么？

<details>
<summary>答案与复习指引</summary>

**A 行编译错误。** C++ 规定 `goto` 不能跳过带析构函数的变量的初始化（如 `std::string`）。因为跳过构造意味着 `s` 未初始化，但离开作用域时仍会析构——UB。

**和 C 的区别：** C 没有构造/析构概念，`goto` 可以跳过任何声明。C++ 的限制是 RAII 的延伸——保护对象生命周期完整性。

**教训：** C++ 中尽量避免 `goto`，用 `break`/`continue`/`return`/异常替代。

**复习：** → [5.5 跳转语句](./5.5-跳转语句.md)
</details>
