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
