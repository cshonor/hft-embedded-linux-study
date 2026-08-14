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
