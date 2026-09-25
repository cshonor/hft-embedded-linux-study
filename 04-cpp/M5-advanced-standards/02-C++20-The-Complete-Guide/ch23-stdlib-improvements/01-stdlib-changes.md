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

<details>
<summary>参考答案</summary>

1. 它返回无符号整数二进制表示中 **1 的个数**（population count），定义在 `<bit>`，通常直接映射到单条 `POPCNT` 指令，比手写循环快得多。
HFT 中的用途：
   - 统计位图里的**活跃订单/持仓数**（`int active = std::popcount(order_bitmap);`）；
   - 权限、状态、特性标志的计数与校验（配合 `has_single_bit` 判断"是否恰好一个"）；
   - 与 `countr_zero` / `countl_zero`（找最低/最高置位）、`bit_width`、`rotl`/`rotr`、`bit_cast`、`std::endian` 一起构成零开销的位工具箱，用于紧凑的订单簿状态与协议编解码。
2. 两点区别：
   1. **精度**：`std::numbers::pi` 是 `double` 能表示的**最接近 π 的值**（完整双精度），手写 `3.14159` 只有 6 位有效数字，误差在长时间累计/高灵敏度计算里会被放大。
   2. **类型安全与一致性**：`<numbers>` 提供的是模板变量（如 `std::numbers::pi_v<float>` / `pi_v<long double>`），可按模板参数取不同精度；且名字统一、不会有人写成 `3.14159` 有人写成 `3.14`。
此外还有 `e`、`ln2`、`log2e`、`sqrt2`、`inv_pi`（1/π）、`phi` 等，全部是编译期常量，不占运行期成本。
3. 优势有四点：
   1. **类型安全、结构化**：`source_location` 是一个对象，提供 `file_name()`、`line()`、`column()`、`function_name()`，而不是文本宏拼接；
   2. **调用方零改动**：利用默认实参即可自动捕获**调用点**：
```cpp
void on_error(std::string_view msg,
              std::source_location loc = std::source_location::current());
on_error("bad tick");   // 自动带上调用点的文件/行号
```
用宏则必须把 `__FILE__`/`__LINE__` 包进宏里，调用方得写宏名，且宏不能做默认参数。
   3. **可传递、可存储**：能作为参数传给下层、塞进日志结构，便于结构化日志；
   4. **不受宏影响**：不会因为宏展开、`-D`、或条件编译而失真，也不污染命名空间。
4. 它解决**多线程写同一个流时输出交错**的问题。
`std::osyncstream`（`<syncstream>`）在内部先缓冲所有写入，到**析构时**（或显式调用 `emit()`）**一次性原子地**刷到目标流；因此每个线程的整条日志是连续的一块，不会出现 `std::cout` 那种"两个线程的字符交错在同一行"的乱序。
```cpp
void worker(int id) {
    std::osyncstream out(std::cout);
    out << "Thread " << id << " working
";   // 析构时一次性 flush
}
```
代价是每条日志一次缓冲与一次同步 flush，所以它保证的是**原子性**而不是速度——高频日志仍应走专门的异步日志队列。
5. C++20 前只能手写：
```cpp
// starts_with
bool sw = s.rfind(prefix, 0) == 0;                 // 或 s.compare(0, n, prefix) == 0
// ends_with
bool ew = s.size() >= suffix.size() &&
          s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
```
写法冗长、容易漏掉"长度不足"的判断（`ends_with` 尤其容易越界）。
C++20 直接提供 `starts_with` / `ends_with`，`std::string` 与 `std::string_view` 都支持，参数可以是 `string_view` 或单个 `char`，语义清晰且不会越界。

</details>
