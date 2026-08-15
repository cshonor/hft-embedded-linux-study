# Item 39：使谓词仿函数可配对

> 第 6 章 仿函数与函数 · Item 39 · 上一节：[Item 38 按值传递](item38-functor-by-value.md) · 下一节：[Item 40-42 函数适配器](item40-42-function-adapters.md)

## 为什么要学这个（先建立直觉）

在 C 里，判断条件就是 `if (x > 5)`——想取反就写 `if (!(x > 5))`，直接改表达式。C++ STL 的谓词是仿函数或函数指针，取反需要"配对"机制。

```c
/* C: 直接取反 */
int is_positive(int x) { return x > 0; }
int is_not_positive(int x) { return !is_positive(x); }
// 或者调用处直接 !is_positive(x)
```

```cpp
// C++: 用 not1/not2 配对器取反
struct IsPositive {
    typedef int argument_type;  // C++03 必须声明这些 typedef
    bool operator()(int x) const { return x > 0; }
};
// C++03: not1(IsPositive{}) 需要 argument_type
// C++11+: lambda 更简单
auto it = std::find_if(v.begin(), v.end(),
    [](int x) { return !(x > 0); });  // 直接在 lambda 里取反
```

**直觉**：谓词是返回 bool 的可调用对象。C++03 要让它能被 `not1`/`not2` 取反，需声明嵌套 typedef。C++11 后 lambda 让"配对"需求基本消失。

## 这节讲什么

**谓词（predicate）** = 返回 `bool` 的仿函数/函数指针。

可配对（adaptable）谓词需要满足：
1. 返回 `bool`（或可隐式转为 bool）
2. `operator()` 是 `const`
3. C++03：声明 `argument_type`（一元）或 `first_argument_type`/`second_argument_type`（二元）

### C++03 可配对谓词

```cpp
#include <functional>

// 一元谓词
struct IsEven : std::unary_function<int, bool> {
    bool operator()(int x) const { return x % 2 == 0; }
};
// std::unary_function<int, bool> 自动提供 argument_type = int, result_type = bool

// 取反
auto it = std::find_if(v.begin(), v.end(), std::not1(IsEven{}));
// 等价于找奇数
```

### C++11 后：lambda 让配对过时

```cpp
// C++03 方式（繁琐）
struct IsEven : std::unary_function<int, bool> {
    bool operator()(int x) const { return x % 2 == 0; }
};
std::find_if(v.begin(), v.end(), std::not1(IsEven{}));

// C++11 方式（简单）
std::find_if(v.begin(), v.end(), [](int x) { return x % 2 != 0; });
// 直接写否定逻辑，不需要 not1
```

### std::function 和 std::not_fn（C++17）

```cpp
// C++17: std::not_fn 取反任意可调用对象
auto is_even = [](int x) { return x % 2 == 0; };
auto is_odd = std::not_fn(is_even);  // C++17
```

## 常见错误（新手踩坑）

### 错误 1：谓词返回非 bool

```cpp
struct Bad {
    int operator()(int x) const { return x; }  // 返回 int 不是 bool
};
// 可以隐式转 bool，但不是谓词的最佳实践
std::find_if(v.begin(), v.end(), Bad{});
```

**修复**：显式返回 bool。

```cpp
struct Good {
    bool operator()(int x) const { return x != 0; }
};
```

### 错误 2：operator() 不是 const

```cpp
struct BadPred {
    mutable int count = 0;
    bool operator()(int x) { count++; return x > 0; }  // non-const!
};
// std::not1(BadPred{}) 可能编译失败
```

**修复**：加 const。

```cpp
struct GoodPred {
    mutable int count = 0;
    bool operator()(int x) const { count++; return x > 0; }
};
```

### 错误 3：C++03 代码忘了继承 unary_function

```cpp
// C++03: 缺少 argument_type
struct IsPositive {
    bool operator()(int x) const { return x > 0; }
};
// std::not1(IsPositive{}) 编译失败：找不到 argument_type
```

**修复**：继承 `std::unary_function<int, bool>` 或手写 typedef。

```cpp
struct IsPositive : std::unary_function<int, bool> {
    bool operator()(int x) const { return x > 0; }
};
// 现在 not1 能用了
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 谓词 | 函数返回 int（0/非0） | 仿函数/lambda 返回 bool |
| 取反 | `!func(x)` 直接写 | `not1`/`not2`（C++03）或 lambda 内取反（C++11+） |
| 类型要求 | 无 | C++03 需 argument_type typedef；C++11 无此要求 |
| const | 函数天然 const | `operator()` 需显式声明 const |

## HFT 关联

- **lambda 直接写否定逻辑**：`[&](const Order& o){ return !o.is_active(); }` 比 `not1` 配对更清晰
- **谓词必须是纯函数**：HFT 过滤逻辑不应有副作用，const operator() 强制无副作用语义
- **C++17 not_fn**：如果需要通用取反，`std::not_fn` 比 `not1` 更灵活，支持任意可调用对象

## 代码自测

### Q1: not1 需要什么

```cpp
struct GreaterThan {
    bool operator()(int x) const { return x > threshold; }
    int threshold;
};
// std::not1(GreaterThan{5}) 能编译吗？
```

<details>
<summary>答案</summary>

**不能编译**。`not1` 要求谓词声明 `argument_type` typedef（C++03 可配对要求）。`GreaterThan` 没有声明它。

修复（C++03）：
```cpp
struct GreaterThan : std::unary_function<int, bool> {
    int threshold;
    explicit GreaterThan(int t) : threshold(t) {}
    bool operator()(int x) const { return x > threshold; }
};
```

C++11+ 直接用 lambda：`[](int x) { return x <= 5; }`
</details>

### Q2: 谓词返回值

```cpp
struct Checker {
    operator bool() const { return true; }  // 这能做谓词吗？
    bool operator()(int x) const { return x > 0; }  // 这才是谓词
};
```
> `Checker` 中的两个函数哪个和谓词有关？

<details>
<summary>答案</summary>

`bool operator()(int x) const` 是谓词——它是调用运算符，返回 bool。

`operator bool()` 是**转换运算符**，把 `Checker` 对象转为 bool，和谓词无关。

谓词的核心是 `operator()` 返回 bool，不是对象本身能转 bool。
</details>

### Q3: lambda 取反

```cpp
std::vector<int> v = {1, -2, 3, -4, 5};
// 找第一个非正数，用 lambda 写
auto it = std::find_if(v.begin(), v.end(), /* ??? */);
```

<details>
<summary>答案</summary>

```cpp
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x <= 0; });
// it 指向 -2
```

不需要 `not1`，直接在 lambda 内写否定逻辑。这是 C++11+ 的推荐做法。
</details>

### Q4: C++17 not_fn

```cpp
auto is_positive = [](int x) { return x > 0; };
auto is_not_positive = std::not_fn(is_positive);
std::vector<int> v = {1, -2, 3};
auto it = std::find_if(v.begin(), v.end(), is_not_positive);
// it 指向哪个元素？
```

<details>
<summary>答案</summary>

`it` 指向 **-2**（第一个非正数）。

`std::not_fn`（C++17）包装 `is_positive`，返回取反结果。`is_not_positive(x)` = `!is_positive(x)` = `!(x > 0)` = `x <= 0`。

`not_fn` 比旧的 `not1`/`not2` 更通用——不需要 `argument_type` typedef，适用于任何可调用对象。
</details>

## 参考与延伸

- 上一节：[Item 38 按值传递](item38-functor-by-value.md)
- 下一节：[Item 40-42 函数适配器](item40-42-function-adapters.md)
- [Effective Modern C++ ch06 lambda](../../../M1-modern-cpp/01-Effective-Modern-C++/ch06-lambda-expressions/README.md)
