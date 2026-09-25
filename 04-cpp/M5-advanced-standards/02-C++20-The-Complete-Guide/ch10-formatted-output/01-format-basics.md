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

<details>
<summary>参考答案</summary>

1. 相比 **printf**：
   - **类型安全**：格式串与实参在编译期校验（占位符与实参个数/类型不匹配是**编译错误**，而 `printf("%s", 42)` 是未定义行为/崩溃）；
   - 不用记 `%d/%s/%llu`，类型自动推导，也支持自定义类型；
   - 支持**位置参数**（`{0} {1} {0}`）便于重复引用与本地化。
相比 **iostream**：
   - **更简洁**：格式与文本写在一处，不必写一长串 `<<` 与操纵符；
   - **更快**：没有 iostream 的 facet/locale 机制与流状态开销（`std::format` 默认不套用全局 locale）；
   - **无状态污染**：iostream 的宽度/精度/填充是"粘性"的，`format` 每次都是显式指定。
此外还有 `format_to`（写入已有缓冲，零分配）与可复用的 `vformat`。
2. `{:<10}` 是**左对齐**、宽度 10（内容靠左，右侧补空格，得到 `"42        "`）；
`{:>10}` 是**右对齐**、宽度 10（内容靠右，左侧补空格，得到 `"        42"`）——对数字来说 `>` 本就是默认对齐。
此外 `{:^10}` 是**居中**；对齐符前可加填充字符，如 `{:*<10}` → `"42********"`、`{:0>10}` → `"0000000042"`。
3. 精度写在 `.` 之后：`{:.N}`。
`{:.2f}` 的含义是"**定点（fixed）记法 + 小数点后保留 2 位**"，所以 `3.14159` 格式化为 `"3.14"`（会四舍五入）。
注意类型字符的作用：`f` = 定点、`e` = 科学计数、`g` = 通用（默认）。若只写精度不写类型（如 `{:.2}`），对浮点表示"**最多 2 位有效数字**"的通用格式，与 `{:.2f}` 的含义不同。
4. 为你的类型**特化 `std::formatter<T>`**，实现两个成员函数：
```cpp
template <>
struct std::formatter<Order> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();            // 不解析自定义说明时原样返回
    }
    auto format(const Order& o, std::format_context& ctx) const {
        return std::format_to(ctx.out(),
            "Order{{sym={}, price={}, qty={}}}", o.sym, o.price, o.qty);
    }
};
```
要点：`parse` 解析 `{}` 中冒号后的说明并返回解析结束位置（通常是 `constexpr`）；`format` 把结果写到 `ctx.out()` 并**返回新的输出迭代器**。特化好后 `std::format("{}", ord)` 就能直接用。
5. 用 `std::format_to` 把结果直接写进**预分配的缓冲区**，不创建 `std::string`、不做堆分配：
```cpp
char buf[256];
auto it = std::format_to(buf, "[{}] latency={}ns sym={}",
                         timestamp(), latency_ns, sym);
*it = '\0';      // 需要 C 字符串时自己补终止符
log(buf);
```
`format_to` 返回写入结束处的迭代器，可继续追加；若担心溢出，用 `std::format_to_n(buf, n, fmt, ...)`（带长度上限）。这样热路径上的日志格式化就是确定无分配的。

</details>
