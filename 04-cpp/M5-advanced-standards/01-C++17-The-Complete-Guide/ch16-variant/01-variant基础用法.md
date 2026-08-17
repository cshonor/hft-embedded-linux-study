# 15.3 std::variant 基础用法

> 第 16 章 std::variant · 下一节：[16.2 variant 访问与陷阱](02-variant访问与陷阱.md)

## 这节讲什么

`std::variant<T1, T2, ...>` 是类型安全的联合体（tagged union）。C 的 union 不记录当前存的是哪种类型，variant 在编译期记录所有可能的类型，运行期跟踪当前类型。

## 为什么要学这个（先建立直觉）

C 的 union：

```c
// C：union 不记录当前存的是哪种类型
union Data {
    int i;
    double d;
    char s[16];
};

struct TaggedData {
    int type;  // 0=int, 1=double, 2=string
    union Data data;
};

// 问题：忘记设 type 或设错 type → UB
struct TaggedData d;
d.data.i = 42;
// d.type = 0;  // 忘了设！
// 后面读 d.data.d → UB
```

C++17 variant：

```cpp
// C++17：variant 自动跟踪当前类型
std::variant<int, double, std::string> v;
v = 42;           // 当前是 int
v = 3.14;         // 当前是 double（自动切换）
v = "hello"s;     // 当前是 string

// 安全访问
if (std::holds_alternative<int>(v)) {
    std::cout << std::get<int>(v);
}
// 类型错误时抛异常而不是 UB
std::get<double>(v);  // 抛 std::bad_variant_access
```

## 基本接口

### 创建与赋值

```cpp
std::variant<int, double, std::string> v;

v = 42;           // 设为 int
v = 3.14;         // 切换为 double
v = "hello"s;     // 切换为 string

// 指定索引构造
std::variant<int, double> v2 = std::in_place_index<0>;  // 默认构造 int=0
```

### 访问

```cpp
std::variant<int, double, std::string> v = 42;

// 1. 按类型访问
std::get<int>(v);       // 42（类型错误抛异常）

// 2. 按索引访问
std::get<0>(v);         // 42（索引错误抛异常）

// 3. 安全获取指针
if (auto p = std::get_if<int>(&v)) {
    std::cout << *p;    // 42
}

// 4. 检查当前类型
std::holds_alternative<int>(v);    // true
std::holds_alternative<double>(v); // false
v.index();  // 0（当前类型的索引）
```

### visit（模式匹配）

```cpp
std::variant<int, double, std::string> v = "hello"s;

// 用 Overloaded lambda（见 ch14）
std::visit(Overloaded{
    [](int x) { std::cout << "int: " << x << "\n"; },
    [](double x) { std::cout << "double: " << x << "\n"; },
    [](const std::string& s) { std::cout << "string: " << s << "\n"; }
}, v);
// 输出: string: hello
```

## 实际用法

### 1. 多态消息体（不用继承）

```cpp
struct OrderMsg { int id; double price; int qty; };
struct TradeMsg { int trade_id; double price; int qty; };
struct CancelMsg { int order_id; };

using Message = std::variant<OrderMsg, TradeMsg, CancelMsg>;

// 接收消息
Message msg = receive();

// 分发处理
std::visit(Overloaded{
    [](const OrderMsg& m) { handle_order(m); },
    [](const TradeMsg& m) { handle_trade(m); },
    [](const CancelMsg& m) { handle_cancel(m); }
}, msg);
```

### 2. 配合 if-init

```cpp
if (auto msg = receive(); std::holds_alternative<OrderMsg>(msg)) {
    auto& order = std::get<OrderMsg>(msg);
    process(order);
}
```

### 3. 状态机

```cpp
using State = std::variant<
    struct Idle,
    struct Connecting,
    struct Connected,
    struct Error
>;

State current = Idle{};

// 状态转换
current = Connecting{};
std::visit(Overloaded{
     { /* 初始状态 */ },
     { /* 连接中 */ },
     { /* 已连接 */ },
     { /* 错误 */ }
}, current);
```

## variant vs 继承

```cpp
// 方法 1：继承 + 虚函数
struct MsgBase { virtual void process() = 0; virtual ~MsgBase() = default; };
struct OrderMsg : MsgBase { void process() override { /* ... */ } };
// 问题：需要动态分配、虚函数开销、指针管理

// 方法 2：variant
using Msg = std::variant<OrderMsg, TradeMsg, CancelMsg>;
// 优点：值语义、无动态分配、无虚函数、编译期分发
```

| 特性 | 继承+虚函数 | variant |
|------|-----------|---------|
| 内存 | 堆分配 | 栈上值语义 |
| 分发 | 运行时（vtable） | 编译时（visit） |
| 扩展 | 加子类 | 加类型到 variant |
| 安全 | 可能忘记实现 | 编译期检查所有分支 |

## HFT 关联

```cpp
// 消息处理：variant 替代继承
struct NewOrder { uint32_t id; double price; uint32_t qty; uint8_t side; };
struct ModifyOrder { uint32_t id; double new_price; uint32_t new_qty; };
struct CancelOrder { uint32_t id; };

using OrderAction = std::variant<NewOrder, ModifyOrder, CancelOrder>;

// 零虚函数开销的消息分发
void process_action(const OrderAction& action) {
    std::visit(Overloaded{
        [](const NewOrder& o) { match(o); },
        [](const ModifyOrder& o) { modify(o); },
        [](const CancelOrder& o) { cancel(o); }
    }, action);
}
```

## 小结

| 接口 | 说明 |
|------|------|
| `std::get<T>(v)` | 按类型访问（错误抛异常） |
| `std::get_if<T>(&v)` | 安全获取指针（错误返回 nullptr） |
| `std::holds_alternative<T>(v)` | 检查当前类型 |
| `v.index()` | 当前类型的索引 |
| `std::visit(visitor, v)` | 模式匹配 |

---

← [本章导读](./README.md) · [下一节 →](02-variant访问与陷阱.md)
