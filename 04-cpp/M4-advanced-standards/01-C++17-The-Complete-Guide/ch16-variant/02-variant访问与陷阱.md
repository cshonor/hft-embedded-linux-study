# 16.2 variant 访问与陷阱

> 第 16 章 std::variant · 上一节：[16.1 variant 基础用法](01-variant基础用法.md)

## 这节讲什么

本节深入 variant 的访问机制、valueless_by_exception 状态、以及使用中的常见陷阱。

## visit 详解

### 泛型 lambda visit

```cpp
std::variant<int, double, std::string> v = 42;

// 泛型 lambda：所有类型统一处理
std::visit([](const auto& x) {
    std::cout << x << "\n";
}, v);
// 要求所有类型都支持 <<
```

### Overloaded 模式

```cpp
// C++17 Overloaded（见 ch14）
template<typename... Fs>
struct Overloaded : Fs... { using Fs::operator()...; };
template<typename... Fs> Overloaded(Fs...) -> Overloaded<Fs...>;

// 使用
std::visit(Overloaded{
    [](int x) { std::cout << "int\n"; },
    [](double x) { std::cout << "double\n"; },
    [](const std::string& s) { std::cout << "string\n"; }
}, v);
```

### visit 返回值

```cpp
// visit 的返回值：所有 lambda 的返回类型必须相同
auto result = std::visit(Overloaded{
    [](int x) -> std::string { return "int: " + std::to_string(x); },
    [](double x) -> std::string { return "double: " + std::to_string(x); },
    [](const std::string& s) -> std::string { return "string: " + s; }
}, v);
// result 是 std::string
```

## valueless_by_exception

### 什么时候会出现

```cpp
std::variant<int, std::string> v = 42;

// 类型切换时构造函数抛异常
try {
    v = std::string("hello");  // 如果 string 构造抛异常
    // 此时旧的 int 已被销毁，新的 string 没构造成功
    // variant 进入 valueless_by_exception 状态
} catch (...) {
    v.valueless_by_exception();  // 可能是 true
}
```

### 检查和处理

```cpp
if (v.valueless_by_exception()) {
    // variant 没有任何值——极少见但可能发生
    // 处理：重置
    v = 0;  // 重新赋值
}

// 访问 valueless 的 variant
// std::get<int>(v);  // 抛异常
// v.index();         // 返回 variant_npos
```

## 常见陷阱

### 陷阱 1：类型推导歧义

```cpp
std::variant<int, long, double> v = 42;
// v 存的是 int 还是 long？42 是 int，所以是 int

std::get<int>(v);   // OK
std::get<long>(v);  // 抛异常（当前是 int 不是 long）

// 但赋值时可能有歧义
std::variant<int, long> v2 = 42;
// 42 是 int，OK

std::variant<long, int> v3 = 42;
// 42 是 int → v3 存 int（第二个类型）
// 但如果两个类型都能从 42 构造呢？
std::variant<long, long long> v4 = 42;
// 歧义！编译错误
```

### 陷阱 2：相同类型

```cpp
// 不能有相同类型
// std::variant<int, int> v;  // 编译错误

// 但可以有 const 修饰
std::variant<int, const int> v;  // 可能编译通过但行为奇怪
```

### 陷阱 3：引用类型

```cpp
// variant 不能直接持有引用
// std::variant<int&, double&> v;  // 编译错误

// 变通：用 reference_wrapper
int x = 42;
std::variant<std::reference_wrapper<int>> v = std::ref(x);
```

### 陷阱 4：数组类型

```cpp
// variant 不能直接持有 C 数组
// std::variant<int[10]> v;  // 编译错误

// 变通：用 std::array
std::variant<std::array<int, 10>> v;
```

### 陷阱 5：visit 漏掉类型

```cpp
std::variant<int, double, std::string> v;

// 漏掉 string 的处理
std::visit(Overloaded{
    [](int) { },
    [](double) { }
    // 漏了 string！
}, v);
// 编译错误！visit 要求处理所有类型
```

## variant 的大小

```cpp
// variant 的大小 = max(sizeof(Ts)...) + 标志位
std::variant<int, double> v1;  // sizeof ≈ 8 + 1（标志）= 16（对齐）
std::variant<int, std::string> v2;  // sizeof ≈ 32（string 大）+ 1 = 40

// 所有可能类型共享同一块内存
// 同一时刻只存一种类型
```

## HFT 关联

```cpp
// 消息路由：variant 替代虚函数
struct MarketData { /* ... */ };
struct OrderUpdate { /* ... */ };
struct Heartbeat { /* ... */ };

using FeedMessage = std::variant<MarketData, OrderUpdate, Heartbeat>;

// 编译期分发，零虚函数开销
void on_message(const FeedMessage& msg) {
    std::visit(Overloaded{
        [this](const MarketData& md) { on_market_data(md); },
        [this](const OrderUpdate& ou) { on_order_update(ou); },
        [this](const Heartbeat& hb) { on_heartbeat(hb); }
    }, msg);
}

// variant 在 SPSC 队列中传递——值语义，无动态分配
RingBuffer<FeedMessage> queue(65536);
```

## 小结

| 陷阱 | 说明 |
|------|------|
| valueless_by_exception | 类型切换时构造抛异常导致 |
| 类型歧义 | 多个类型能从同一值构造 |
| 不支持引用 | 用 reference_wrapper |
| visit 必须全覆盖 | 漏掉类型编译错误 |
| 大小 = max(Ts) + 标志 | 所有类型共享内存 |

---

← [上一节](01-variant基础用法.md) · [本章导读](./README.md)
