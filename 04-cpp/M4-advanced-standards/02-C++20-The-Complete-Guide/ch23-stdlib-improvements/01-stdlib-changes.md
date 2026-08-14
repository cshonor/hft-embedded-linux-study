# 标准库改进

## <bit> 头

```cpp
#include <bit>

// 位操作
std::popcount(0b1011u);   // 3（1 的个数）
std::countl_zero(0u);     // 32（前导零个数）
std::countr_zero(1u);     // 0（尾部零个数）
std::has_single_bit(8u);  // true（8 是 2 的幂）
std::bit_ceil(5u);        // 8（>= 5 的最小 2 的幂）
std::bit_floor(5u);       // 4（<= 5 的最大 2 的幂）
std::bit_width(5u);       // 3（表示 5 需要的位数）

// 位旋转
std::rotl(0b0001u, 2);    // 0b0100（左旋 2 位）
std::rotr(0b0100u, 2);    // 0b0001（右旋 2 位）

// byteswap（C++23）
// std::byteswap<uint16_t>(0x1234);  // 0x3412
```

## <numbers> 头

```cpp
#include <numbers>

// 数学常量（C++20）
std::numbers::pi;           // 3.14159265...
std::numbers::e;            // 2.71828182...
std::numbers::sqrt2;        // 1.41421356...
std::numbers::ln2;          // 0.69314718...
std::numbers::log2e;        // 1.44269504...
```

## <source_location>

```cpp
#include <source_location>

void log_msg(const std::string& msg,
             const std::source_location& loc = std::source_location::current()) {
    std::cout << loc.file_name() << ':' << loc.line()
              << " [" << loc.function_name() << "] "
              << msg << '\n';
}

log_msg("error occurred");
// 输出：main.cpp:10 [main] error occurred
```

## <syncstream>

```cpp
#include <syncstream>

// 同步输出流：线程安全的 cout
void worker(int id) {
    std::osyncstream out(std::cout);  // 线程安全
    out << "Thread " << id << " working\n";
    // 析构时一次性刷新——不会交错
}
```

## 字符串改进

```cpp
// starts_with / ends_with
std::string s = "hello world";
s.starts_with("hello");  // true
s.ends_with("world");    // true

// string_view 也支持
std::string_view sv = "hello";
sv.starts_with("he");    // true
```

## HFT 应用

```cpp
// <bit> 优化位操作
int active_orders = std::popcount(order_bitmap);  // 快速计算活跃订单数

// <numbers> 精确常量
double black_scholes(/* ... */) {
    return /* ... */ * std::numbers::inv_pi;  // 1/π
}

// <source_location> 日志
void on_error(const std::string& msg,
              const std::source_location& loc = std::source_location::current()) {
    log("[{}:{}] {}", loc.file_name(), loc.line(), msg);
}

// <syncstream> 线程安全日志
std::osyncstream log(std::cout);
log << "Order filled: " << order_id << '\n';
```

## 自测题

1. `std::popcount` 做什么？HFT 中有什么用？
2. `std::numbers::pi` 和手写 `3.14159` 有什么区别？
3. `std::source_location` 相比 `__FILE__`/`__LINE__` 有什么优势？
4. `std::osyncstream` 解决什么问题？
5. `starts_with`/`ends_with` 在 C++20 前怎么实现？
