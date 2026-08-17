# std::format 基础

## 基本用法

```cpp
#include <format>

// 类似 Python 的 format
std::string s = std::format("Hello, {}! You are {} years old.", "Alice", 30);
// "Hello, Alice! You are 30 years old."

// 位置参数
std::format("{0} vs {1} vs {0}", "A", "B");
// "A vs B vs A"

// 输出到流
std::cout << std::format("x = {}, y = {}\n", x, y);

// 输出到迭代器
char buf[100];
auto it = std::format_to(buf, "x = {}", 42);
*it = '\0';
```

## 格式说明符

```cpp
// 语法：{[位置]:[填充][对齐][符号][#][0][宽度][.精度][类型]}

// 宽度
std::format("{:10}", 42);     // "        42"（右对齐，宽10）
std::format("{:<10}", 42);    // "42        "（左对齐）
std::format("{:^10}", 42);    // "    42    "（居中）
std::format("{:>10}", 42);    // "        42"（右对齐，默认）

// 填充字符
std::format("{:0>10}", 42);   // "0000000042"
std::format("{:*<10}", 42);   // "42********"

// 精度（浮点）
std::format("{:.2f}", 3.14159);  // "3.14"
std::format("{:.4f}", 3.14159);  // "3.1416"

// 类型
std::format("{:d}", 42);      // 十进制 "42"
std::format("{:x}", 255);     // 十六进制 "ff"
std::format("{:o}", 255);     // 八进制 "377"
std::format("{:b}", 255);     // 二进制 "11111111"
std::format("{:e}", 3.14);    // 科学计数 "3.140000e+00"
```

## 对比 printf / iostream

```cpp
// printf：不安全、不类型检查
printf("%d %s\n", 42, "hello");
printf("%s %d\n", 42);  // 崩溃（42 当字符串）

// iostream：慢、冗长
std::cout << "x = " << x << ", y = " << y << std::endl;

// std::format：安全、简洁、高效
std::format("x = {}, y = {}\n", x, y);
// 类型安全（编译期检查）、无 iostream 开销
```

## 自定义类型格式化

```cpp
// C++20 自定义 formatter
template <>
struct std::formatter<Order> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Order& o, format_context& ctx) const {
        return std::format_to(ctx.out(), "Order{{sym={}, price={}, qty={}}}",
            o.sym, o.price, o.qty);
    }
};

Order ord{"AAPL", 150.25, 100};
std::format("{}", ord);  // "Order{sym=AAPL, price=150.25, qty=100}"
```

## HFT 应用

```cpp
// 日志格式化（零分配版用 format_to）
char buf[256];
auto it = std::format_to(buf, "[{}] latency={}ns sym={}",
    timestamp(), latency_ns, sym);
*it = '\0';
log(buf);

// 多字段输出
std::string msg = std::format("FILL {} {}@{} order_id={}",
    side == BUY ? "BUY" : "SELL", qty, price, order_id);
```

## 自测题

1. `std::format` 相比 `printf` 和 `iostream` 有什么优势？
2. 格式说明符 `{:<10}` 和 `{:>10}` 分别是什么对齐？
3. 如何指定浮点精度？`{:.2f}` 是什么意思？
4. 如何自定义类型的 `formatter`？
5. HFT 日志如何用 `format_to` 实现零分配格式化？
