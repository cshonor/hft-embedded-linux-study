# 14.1 扩展 using 声明

> 第 14 章 扩展 using 声明

## 这节讲什么

C++17 扩展了 `using` 声明，允许变长 using 包展开（pack expansion）。这在继承构造函数和组合模式中非常有用——一行 `using` 就能把基类或成员的多个重载引入派生类。

## C++14 的限制

```cpp
// C++14：using 声明不能展开参数包
template<typename... Bases>
struct Derived : Bases... {
    // C++14：需要逐个写
    using Bases::f...;  // ❌ C++14 不支持
};

// 变通方案：递归继承
template<typename Base>
struct Wrapper1 : Base {
    using Base::f;
};

template<typename B1, typename B2>
struct Wrapper2 : B1, B2 {
    using B1::f;
    using B2::f;
};
```

## C++17 的变长 using

```cpp
// C++17：using 声明可以展开参数包
template<typename... Bases>
struct Derived : Bases... {
    using Bases::f...;  // ✅ C++17: 展开所有基类的 f
};

struct A { void f(int) {} };
struct B { void f(double) {} };
struct C { void f(const char*) {} };

Derived<A, B, C> d;
d.f(1);        // A::f(int)
d.f(1.0);      // B::f(double)
d.f("hello");  // C::f(const char*)
```

## 继承构造函数的变长 using

```cpp
template<typename... Bases>
struct Composed : Bases... {
    using Bases::Bases...;  // 继承所有基类的构造函数
};

struct A {
    A(int) {}
};
struct B {
    B(double) {}
};

Composed<A, B> c1(42);    // A(int)
Composed<A, B> c2(3.14);  // B(double)
```

## 实际用法

### 1. Mixin 组合

```cpp
// 可组合的 Mixin
template<typename... Mixins>
class GameObject : public Mixins... {
public:
    using Mixins::Mixins...;  // 继承所有 Mixin 的构造函数
    using Mixins::update...;  // 引入所有 Mixin 的 update 方法
};

struct PositionMixin {
    double x, y;
    void update(double dt) { x += vx * dt; y += vy * dt; }
};

struct HealthMixin {
    int hp;
    void update(double dt) { regen(dt); }
};

// 组合
using Enemy = GameObject<PositionMixin, HealthMixin>;
Enemy e;
e.PositionMixin::update(0.016);  // 调用 PositionMixin 的 update
e.HealthMixin::update(0.016);    // 调用 HealthMixin 的 update
```

### 2. 类型列表展开

```cpp
// 从多个基类引入类型别名
template<typename... Ts>
struct TypeList : Ts... {
    using Ts::value_type...;  // 引入所有基类的 value_type
};

// 注意：如果多个基类有同名 typedef，会有歧义
```

### 3. 重载集合并

```cpp
// 合并多个重载集
template<typename... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;  // C++17: 展开所有 operator()
};

// C++17 构造语法（CTAD）
template<typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

// 使用：合并多个 lambda 为一个可调用对象
auto visitor = Overloaded{
    [](int x) { std::cout << "int: " << x << "\n"; },
    [](double x) { std::cout << "double: " << x << "\n"; },
    [](const std::string& s) { std::cout << "string: " << s << "\n"; }
};

visitor(42);           // int: 42
visitor(3.14);         // double: 3.14
visitor("hello"s);     // string: hello
```

### 4. variant 访问器

```cpp
// Overloaded + visit = 类型安全的 variant 访问
std::variant<int, double, std::string> v = 42;

std::visit(Overloaded{
    [](int x) { std::cout << "int\n"; },
    [](double x) { std::cout << "double\n"; },
    [](const std::string& s) { std::cout << "string\n"; }
}, v);
// 输出: int
```

## 注意事项

### 名字冲突

```cpp
struct A { void f(int) {} };
struct B { void f(int) {} };  // 同签名的 f

struct C : A, B {
    using A::f;
    using B::f;  // C++17: OK，引入两个同签名的 f
    // 但调用时会有歧义
};

// C c;
// c.f(1);  // 歧义！A::f 还是 B::f？
// 需要：c.A::f(1) 或 c.B::f(1)
```

### 访问权限

```cpp
struct Base {
private:
    void f(int);
public:
    void f(double);
};

struct Derived : Base {
    using Base::f;  // 只引入 public f(double)，private f(int) 不可访问
};
```

## HFT 关联

```cpp
// 消息处理器组合
struct OrderHandler {
    void handle(const OrderMsg&) { /* ... */ }
};
struct TradeHandler {
    void handle(const TradeMsg&) { /* ... */ }
};
struct CancelHandler {
    void handle(const CancelMsg&) { /* ... */ }
};

// 一行合并所有 handler
template<typename... Handlers>
struct MessageRouter : Handlers... {
    using Handlers::handle...;
};

using Router = MessageRouter<OrderHandler, TradeHandler, CancelHandler>;
Router router;
// router.handle(order_msg) → OrderHandler::handle
// router.handle(trade_msg) → TradeHandler::handle
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| `using Bases::f...` | ❌ | ✅ |
| `using Bases::Bases...`（构造函数） | ❌ | ✅ |
| `using Fs::operator()...` | ❌ | ✅ |
| Overloaded 模式 | 需要递归 | 一行搞定 |

---

← [本章导读](./README.md)
