# 7.1 仿函数的两大基类
> 第 7 章 仿函数 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[7.2 内置仿函数](02-builtin-functors.md)

## 为什么要学这个（先建立直觉）

C 里函数指针是唯一的"可调用对象"，没有函数对象的概念：

```c
// C: 只有函数指针
int add(int a, int b) { return a + b; }
int (*op)(int, int) = add;
// 函数指针无法携带状态，无法内联，无法组合
// qsort 传入的 cmp 函数指针，每次调用都是间接跳转
```

C++ 的仿函数（functor）是重载了 `operator()` 的类——可以携带状态、可以内联、可以组合：

```cpp
struct Adder {
    int delta;
    Adder(int d) : delta(d) {}
    int operator()(int x) const { return x + delta; }  // 携带状态
};
Adder add5(5);
add5(10);  // 15 —— 像函数一样调用，但是一个对象
```

但要让仿函数能被 STL 适配器（`not1`/`bind2nd`）包装，它需要提供**关联类型**——这就是 `unary_function`/`binary_function` 基类的用途。

## 这节讲什么

SGI STL 定义了两个基类，让仿函数携带参数类型和返回类型信息，从而可被适配器配对。

### 两大基类定义

```cpp
// 一元仿函数基类
template<class Arg, class Result>
struct unary_function {
    typedef Arg    argument_type;   // 参数类型
    typedef Result result_type;     // 返回类型
};

// 二元仿函数基类
template<class Arg1, class Arg2, class Result>
struct binary_function {
    typedef Arg1   first_argument_type;   // 第一个参数类型
    typedef Arg2   second_argument_type;  // 第二个参数类型
    typedef Result result_type;           // 返回类型
};
```

### 为什么需要关联类型？

```cpp
// 没有 base class 的仿函数
struct MyPred {
    bool operator()(int x) const { return x > 5; }
};
// not1(MyPred()) 编译失败！
// 因为 not1 需要知道 argument_type 和 result_type 才能生成否定版

// 继承 unary_function 后
struct MyPred : public std::unary_function<int, bool> {
    bool operator()(int x) const { return x > 5; }
};
// not1(MyPred()) OK！
// not1 从 unary_function 萃取 argument_type=int, result_type=bool
// 生成 !(x > 5) 的新仿函数
```

### 适配器如何萃取类型

```cpp
// not1 的实现（简化）
template<class Predicate>
class unary_negate : public std::unary_function<
    typename Predicate::argument_type,  // 萃取参数类型
    bool>                               // 否定结果恒为 bool
{
    Predicate pred;
public:
    explicit unary_negate(const Predicate& x) : pred(x) {}
    bool operator()(const typename Predicate::argument_type& x) const {
        return !pred(x);  // 否定原谓词
    }
};

template<class Predicate>
unary_negate<Predicate> not1(const Predicate& pred) {
    return unary_negate<Predicate>(pred);
}
// not1 需要从 Predicate 萃取 argument_type
// 所以 Predicate 必须继承 unary_function
```

### C++11 后的变化

```cpp
// C++03: 必须继承基类
struct GreaterThan5 : std::unary_function<int, bool> {
    bool operator()(int x) const { return x > 5; }
};
auto neg = std::not1(GreaterThan5{});  // OK

// C++11: lambda 无需继承
auto neg = std::not1([](int x) { return x > 5; });
// 但实际上 lambda 闭包类型没有 argument_type typedef
// 所以 not1 对 lambda 不工作！

// C++14: 用 std::unary_function 特化或 function
auto neg = std::not1(std::function<bool(int)>([](int x) { return x > 5; }));

// C++17: not_fn 替代 not1/not2
auto neg = std::not_fn([](int x) { return x > 5; });  // OK！
```

C++11 后 `unary_function`/`binary_function` 已被废弃（C++17 删除），因为 `decltype` 和 `auto` 可以推导类型，不再需要手动 typedef。

## 常见错误（新手踩坑）

### 错误 1：自定义仿函数忘记继承基类

```cpp
// ❌ 没有继承 unary_function，适配器不工作
struct IsEven {
    bool operator()(int x) const { return x % 2 == 0; }
};
std::not1(IsEven{});  // 编译错误：找不到 argument_type

// ✅ 继承 unary_function（C++03 方式）
struct IsEven : std::unary_function<int, bool> {
    bool operator()(int x) const { return x % 2 == 0; }
};
std::not1(IsEven{});  // OK
```

### 错误 2：operator() 不是 const

```cpp
// ❌ operator() 不是 const，STL 算法可能编译失败
struct Counter : std::unary_function<int, void> {
    int count = 0;
    void operator()(int x) { ++count; }  // 非 const！修改了成员
};
// STL 算法按值传递仿函数，可能需要 const operator()
// for_each 可以接受非 const，但其他算法可能不行

// ✅ 用 mutable 或设计为 const
struct Counter : std::unary_function<int, void> {
    mutable int count = 0;  // mutable 允许 const 方法修改
    void operator()(int x) const { ++count; }  // const + mutable
};
```

### 错误 3：C++11 后还在用 unary_function

```cpp
// ❌ C++17 已删除 unary_function
struct Pred : std::unary_function<int, bool> { ... };  // C++17 编译错误

// ✅ C++11+ 直接写 lambda
auto pred = [](int x) { return x > 5; };
// 适配器用 not_fn（C++17）而非 not1
auto neg = std::not_fn(pred);
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 只有函数指针 | 仿函数 + lambda + 函数指针 | C++ 多种可调用对象 |
| 函数指针不可内联 | 仿函数/lambda 可内联 | C++ 更快 |
| 无状态携带 | 仿函数可携带状态 | C++ 更灵活 |
| 无可组合性 | 基类关联类型 + 适配器 | C++ 可组合 |
| 无关联类型 | `unary_function`/`binary_function` | C++03 需要，C++11 废弃 |

## HFT 关联

- **理解基类 = 读懂老代码**：STL 源码和 C++03 代码大量使用 `unary_function`，理解它才能读懂
- **新代码用 lambda**：HFT 新代码一律用 lambda——类型唯一可内联、无需继承基类、简洁可读
- **not_fn 替代 not1/not2**：C++17 `std::not_fn` 对任何可调用对象工作，不需要关联类型

## 代码自测

### Q1: unary_function 和 binary_function 提供了什么？

```cpp
struct Plus : std::binary_function<int, int, int> {
    int operator()(int a, int b) const { return a + b; }
};
```
> Plus 继承 binary_function 后获得了什么？这些 typedef 有什么用？

<details>
<summary>答案与复习指引</summary>

**获得了三个 typedef**：
- `first_argument_type` = `int`
- `second_argument_type` = `int`
- `result_type` = `int`

**用途**：让适配器（`bind1st`/`bind2nd`/`not2`）能萃取参数类型，生成新的仿函数。

例如 `bind2nd(Plus(), 5)` 需要知道 `second_argument_type` 才能固定第二个参数为 5：
```cpp
// bind2nd 内部
template<class Operation>
class binder2nd : public unary_function<
    typename Operation::first_argument_type,
    typename Operation::result_type>
{
    Operation op;
    typename Operation::second_argument_type value;  // 需要这个 typedef
    // ...
};
```

**C++11 后**：`decltype`/`auto` 替代手动 typedef，`bind`/lambda 不需要继承基类。

**复习：** → [适配器如何萃取类型](./01-base-classes.md)
</details>

### Q2: 为什么 not1 对 lambda 不工作？

```cpp
// C++03: 仿函数继承基类
struct Greater5 : std::unary_function<int, bool> {
    bool operator()(int x) const { return x > 5; }
};
std::not1(Greater5{});  // OK

// C++11: lambda
auto greater5 = [](int x) { return x > 5; };
// std::not1(greater5);  // 编译错误！lambda 闭包类型没有 argument_type
```
> lambda 闭包类型有 argument_type 吗？怎么解决？

<details>
<summary>答案与复习指引</summary>

**lambda 闭包类型没有 argument_type/result_type**——编译器生成的闭包类不继承 `unary_function`。

**解决方案**：
1. C++14: 包装成 `std::function<bool(int)>`（`function` 有 `argument_type`）
   ```cpp
   std::not1(std::function<bool(int)>(greater5));  // OK，但有开销
   ```
2. C++17: 用 `std::not_fn`（不需要关联类型）
   ```cpp
   std::not_fn(greater5);  // OK，完美转发
   ```
3. 直接写否定逻辑
   ```cpp
   auto not_greater5 = [](int x) { return x <= 5; };  // 最简单
   ```

**教训**：C++11 后 `not1`/`not2` 已过时，用 `not_fn` 或直接写否定 lambda。

**复习：** → [C++11 后的变化](./01-base-classes.md)
</details>

### Q3: operator() 为什么要标记 const？

```cpp
struct BadFunctor {
    int state = 0;
    void operator()(int x) { state += x; }  // 非 const
};

struct GoodFunctor {
    mutable int state = 0;
    void operator()(int x) const { state += x; }  // const + mutable
};
```
> STL 算法传递仿函数的方式是什么？为什么 operator() 最好是 const？

<details>
<summary>答案与复习指引</summary>

**STL 算法按值传递仿函数**（拷贝一份）。算法内部可能用 const 引用调用仿函数：

```cpp
// for_each 内部（简化）
template<class Iter, class F>
F for_each(Iter first, Iter last, F f) {
    for (; first != last; ++first)
        f(*first);  // f 可能是 const
    return f;
}
```

如果 `operator()` 是非 const，且算法用 const 引用调用，编译失败。

**最佳实践**：
- `operator()` 标记 `const`（不修改仿函数状态）
- 需要累积状态用 `mutable` 成员
- 或用引用捕获的 lambda（更简洁）

**HFT**：有状态仿函数用 `for_each` 返回值取最终状态，不要靠副作用。

**复习：** → [operator() 不是 const](./01-base-classes.md)
</details>

### Q4: C++17 为什么删除了 unary_function/binary_function？

```cpp
// C++17 之前可用
struct Pred : std::unary_function<int, bool> { ... };

// C++17 已删除
// std::unary_function<int, bool>  // 编译错误
```
> 删除的原因是什么？什么替代了它们？

<details>
<summary>答案与复习指引</summary>

**删除原因**：
1. `decltype`/`auto` 让编译器自动推导参数和返回类型，不需要手动 typedef
2. `std::bind` 和 lambda 不依赖关联类型
3. `std::function` 用类型擦除，内部自带类型信息
4. `not_fn`（C++17）替代 `not1`/`not2`，不需要 `argument_type`

**替代品**：
- `std::function<Ret(Args...)>`：类型擦除的可调用对象包装器
- `std::bind` + lambda：不需要基类
- `std::not_fn`：不需要 `argument_type`
- `std::is_invocable`/`std::invoke_result`：编译期类型检查

**教训**：新代码不要用 `unary_function`/`binary_function`。理解它们只是为了读老代码和 STL 源码。

**复习：** → [C++11 后的变化](./01-base-classes.md)
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[7.2 内置仿函数](02-builtin-functors.md)
- 源码参考：`bits/stl_function.h`（GCC libstdc++ 的 `unary_function`/`binary_function`）
