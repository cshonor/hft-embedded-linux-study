# is_invocable 系列

## 基本用法

```cpp
#include <type_traits>

void foo(int);
int  bar(double);
struct Obj { void method(int); };

// is_invocable：能否用指定参数调用
static_assert(std::is_invocable_v<decltype(foo), int>);           // true
static_assert(std::is_invocable_v<decltype(foo), const char*>);   // false（const char* → int 需隐式转换）
static_assert(!std::is_invocable_v<decltype(foo), std::string>);  // false

// 成员函数指针
static_assert(std::is_invocable_v<decltype(&Obj::method), Obj&, int>);  // true
```

## is_invocable_r：检查返回类型

```cpp
// is_invocable_r<R, F, Args...>：调用 F(Args...) 的返回值能否转为 R
bool pred(int);
double calc(int);

static_assert(std::is_invocable_r_v<bool, decltype(pred), int>);    // true：pred 返回 bool
static_assert(std::is_invocable_r_v<int, decltype(calc), int>);     // true：double → int 可隐式转
static_assert(!std::is_invocable_r_v<std::string, decltype(pred), int>); // false
```

## is_nothrow_invocable

```cpp
void no_throw(int) noexcept;
void may_throw(int);

static_assert(std::is_nothrow_invocable_v<decltype(no_throw), int>);    // true
static_assert(!std::is_nothrow_invocable_v<decltype(may_throw), int>);  // false
```

## 实际应用：模板约束

```cpp
// 编译期检查回调签名
template <typename Callback>
void register_callback(Callback cb) {
    static_assert(std::is_invocable_r_v<bool, Callback, int>,
                  "Callback must be bool(int)");
    // ...
}

// C++20 Concepts 更好，但 C++17 用 is_invocable 做 SFINAE
template <typename F,
          typename = std::enable_if_t<std::is_invocable_v<F, int>>>
void for_each_int(F f, int x) { f(x); }
```

## 与 invoke 的关系

`is_invocable<F, Args...>` 等价于 "`std::invoke(std::declval<F>(), std::declval<Args>()...)` 是否合法"：
- 普通函数：`f(args...)`
- 成员函数指针：`(obj.*pmf)(args...)` 或 `(ptr->*pmf)(args...)`
- 成员指针：`obj.*pmd` 或 `ptr->*pmd`
- 函数对象：`f(args...)`

`invoke` 统一了这些调用形式，`is_invocable` 基于 `invoke` 做检测。

## 自测题

1. `is_invocable_v<F, int>` 和 `is_invocable_r_v<bool, F, int>` 的区别？
2. 成员函数指针的 `is_invocable` 检查怎么写？
3. `is_nothrow_invocable` 什么时候为 `true`？
4. 为什么 `is_invocable` 基于 `std::invoke` 而不是直接 `f(args...)`？
5. C++17 模板约束用 `is_invocable` + `enable_if`，C++20 的更好方式是什么？

<details>
<summary>参考答案</summary>

1. `is_invocable_v<F, int>` 只判断「`F` 能否以 `int` 实参被调用」（按 INVOKE 语义），**不关心返回类型**，能调用就为 `true`（返回 `void` 也算）。
`is_invocable_r_v<bool, F, int>` 在「能调用」之上**还要求返回类型可隐式转换为 `bool`**；返回 `void` 会判为 `false`。
例：`pred` 返回 `bool`，则 `is_invocable_r_v<bool, decltype(pred), int>` 为 `true`，而 `is_invocable_r_v<std::string, decltype(pred), int>` 为 `false`；`calc` 返回 `double` 时 `is_invocable_r_v<int, decltype(calc), int>` 为 `true`（`double` 可隐式转 `int`）。
2. 按 INVOKE 语义，成员函数指针的第一个「参数」是**对象**：对象本身、引用、指针或智能指针都可以。
```cpp
struct Foo { bool bar(int) const; };
static_assert(std::is_invocable_v<decltype(&Foo::bar), Foo, int>);
static_assert(std::is_invocable_v<decltype(&Foo::bar), Foo&, int>);
static_assert(std::is_invocable_v<decltype(&Foo::bar), const Foo*, int>);
static_assert(std::is_invocable_r_v<bool, decltype(&Foo::bar), Foo&, int>);
```
注意要用 `decltype(&Foo::bar)` 取成员函数指针类型，参数列表里除成员函数自身的参数外还要补上对象（不写 `this`）。
3. 当 `F` 能以给定参数被调用，且该调用表达式被判定为**不抛异常**（`noexcept`）时为 `true`。
也就是要求可调用物本身的调用运算符/函数带 `noexcept`，并且涉及的实参转换、返回值的构造与析构等环节都是 `noexcept`；任一环节可能抛异常就为 `false`。
典型用途：`static_assert(std::is_nothrow_invocable_v<decltype(cb), Tick>)` 保证回调在热路径上不会抛异常。
4. 因为 `is_invocable` 采用标准库的 **INVOKE** 语义，而 `f(args...)` 这种直接调用形式对成员函数指针、数据成员指针根本不成立（`pmf(args...)` 不合法）。
`std::invoke` 统一了普通函数、函数对象/lambda、成员函数指针、数据成员指针这四类可调用物的调用规则，`is_invocable` 复用同一规则，因此覆盖面更广，语义也与 `std::invoke` / `std::invoke_result` 完全一致。
5. C++20 用 concepts 表达更直观，报错信息也更好：
```cpp
template <std::invocable<int> F>
void for_each_int(F f, int x) { f(x); }

// 还要约束返回类型可转 bool：
template <typename F>
  requires std::invocable<F, int> &&
           std::convertible_to<std::invoke_result_t<F, int>, bool>
void register_callback(F cb) { /* ... */ }
```
相比 C++17 的 `enable_if_t<is_invocable_v<...>>`（靠 SFINAE，报错难读、重载集易被污染），constraints 直接约束模板参数，语义清晰，而且参与重载消解。

</details>
