# std::as_const

## 基本用法

```cpp
#include <utility>

std::string s = "hello";
const std::string& cs = std::as_const(s);   // 获得 const 引用

// 等价于：
const std::string& cs2 = const_cast<const std::string&>(s);
// 但 as_const 更安全、更清晰
```

## 解决重载选择问题

```cpp
void foo(std::string&);       // 非 const 重载
void foo(const std::string&); // const 重载

std::string s = "hello";

foo(s);                 // 调非 const 版（s 是左值）
foo(std::as_const(s));  // 调 const 版

// 常见场景：const 容器只返回 const 引用
std::vector<int> v = {1, 2, 3};
auto& ref1 = v[0];              // int&（非 const 重载）
auto& ref2 = std::as_const(v)[0]; // const int&（const 重载）
```

## 为什么不直接 cast？

```cpp
// const_cast 写法——危险，可能去掉 const
const std::string& cs = const_cast<const std::string&>(s);

// as_const 写法——安全，只加 const
const std::string& cs = std::as_const(s);

// as_const 的实现：
template <typename T>
constexpr const T& as_const(T& t) noexcept { return t; }
// 没有 const_cast 去掉 const 的风险
```

## 实际应用

```cpp
// 1. 强制走 const 成员函数
class OrderBook {
public:
    const Price& best_bid() const;
    Price& best_bid();
};

OrderBook book;
// 风控检查：确保不修改行情
const auto& bid = std::as_const(book).best_bid();  // 调 const 版

// 2. 泛型代码中保证 const 语义
template <typename T>
void safe_read(T& container) {
    // 确保 auto& 是 const 引用
    for (const auto& elem : std::as_const(container)) {
        // 只读，不会误修改
    }
}
```

## 自测题

1. `std::as_const` 解决什么问题？
2. 为什么用 `as_const` 而不用 `const_cast`？
3. `as_const(s)` 返回什么类型？
4. 如何用 `as_const` 强制调用 const 成员函数？
5. 泛型代码中 `as_const` 如何保证只读语义？

<details>
<summary>参考答案</summary>

1. 它解决「**给一个非 const 的左值显式加上 const**」的问题——从而强制走 const 重载、拿到 `const T&`、避免误修改。
典型场景：容器有 const/非 const 两套 `operator[]`，`v[0]` 默认拿到 `int&`；写 `std::as_const(v)[0]` 才拿到 `const int&`。在泛型代码里也能用它把 `T&` 变成 `const T&`，而不必改函数签名。
2. `const_cast<const T&>(x)` 是「去掉/加上 cv」的通用工具，用错了会把真正的 const 去掉（例如手滑写成 `const_cast<T&>`），一改就变成未定义行为，而且编译器不会报错。
`std::as_const` 只能**加** const，不能去掉：它就是一个返回 `const T&` 的小函数，语义受限、意图明确；此外 C++17 还显式 `delete` 了 `as_const(const T&&)` 重载，防止把右值/临时对象绑成长命的 const 引用。
3. 返回 `const T&`，即「`T` 的 const 左值引用」。
```cpp
std::string s;
const std::string& cs = std::as_const(s);   // 类型是 const std::string&
```
它等价于 `std::add_const_t<T>&`，`T` 本身若是引用类型则不会发生引用折叠成右值引用的情况——`as_const` 只接受左值。
4. 给对象套一层 `as_const` 再调用，重载解析就会选中 const 版本：
```cpp
class OrderBook {
public:
    const Price& best_bid() const;   // 只读版
    Price&       best_bid();         // 可写版
};

OrderBook book;
const auto& bid = std::as_const(book).best_bid();  // 调用 const 版
```
`std::as_const(book)` 得到 `const OrderBook&`，非 const 成员不可调用，于是只能匹配 const 成员函数——风控/只读检查场景用它把「不修改」这件事在编译期表达出来。
5. 在 `for` 循环或成员访问前套 `as_const`，让推导出的元素类型天然带 const：
```cpp
template <typename T>
void safe_read(T& container) {
    for (const auto& elem : std::as_const(container)) {
        // elem 是 const 的，误写会编译失败
    }
}
```
这样即使 `T` 是非 const 容器，遍历得到的也是 const 引用；同时也能避免 `auto&` 意外绑定非 const 重载。配合 `std::as_const` 的只读语义，修改操作会在编译期被拦住。

</details>
