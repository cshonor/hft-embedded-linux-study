# 第 31 章 to_chars / from_chars

**std::to_chars() and std::from_chars()**

## 本章讲什么

C++17 引入了**最快**的数值↔字符串转换函数：`std::to_chars` 和 `std::from_chars`。无 locale、无异常、无堆分配、无 iostream 开销，HFT 数值格式化的首选。

## 要点

### 为什么需要它们

C++17 之前的转换方案都有代价：

| 方案 | 问题 |
|------|------|
| `std::to_string` | 有 locale 查询、可能堆分配、慢 |
| `sprintf`/`snprintf` | 有 locale、缓冲区安全风险、慢 |
| `std::stoi`/`strtol` | 有 locale、要 null-terminated、设 errno |
| `iostream` << / >> | 极慢（虚函数、locale、sentry） |

`to_chars`/`from_chars` 的设计目标：**零开销、可预测、无 locale、round-trip 保证**。

### `to_chars`：数值 → 字符串

```cpp
#include <charconv>

char buf[32];
auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), 42);   // 整数
// ptr 指向写入末尾，ec 是 errc（成功时为 {}）
*ptr = '\0';   // 不自动 null-terminate！

// 浮点
auto [p2, e2] = std::to_chars(buf, buf + 32, 3.14159);  // 默认最短表示

// 指定格式
std::to_chars(buf, buf+32, 255, std::chars_format::hex);          // 16 进制
std::to_chars(buf, buf+32, 3.14, std::chars_format::scientific, 2); // 科学计数 2 位
```

特点：
- **不 null-terminate**：只写数字字符，ptr 指向末尾，调用方自行补 `\0`。
- **无 locale**：永远用 C locale（小数点 `.`），不随地区变。
- **不抛异常**：错误通过 `ec` 返回。
- **最短表示**：浮点默认写最短且能 round-trip 的表示（C++17 难点，实现如 Ryu 算法）。
- **缓冲区不够**返回 `errc::value_too_large`。

### `from_chars`：字符串 → 数值

```cpp
const char* s = "12345";
int val;
auto [ptr, ec] = std::from_chars(s, s + strlen(s), val);
if (ec == std::errc{}) {
    // val = 12345，ptr 指向未解析部分
}

// 浮点
double d;
std::from_chars(s, s+n, d);

// 16 进制
std::from_chars(s, s+n, val, 16);
```

特点：
- **不跳前导空白**：`from_chars(" 123")` 失败（不同于 `strtol` 会跳空白）。
- **不设 errno**：错误通过 `ec` 返回（`invalid_argument`/`result_out_of_range`）。
- **返回解析位置**：`ptr` 指向第一个未解析字符，方便链式解析。
- **无 locale**。

### round-trip 保证

```cpp
double d = 3.141592653589793;
char buf[64];
to_chars(buf, buf+64, d);   // 写最短能还原的表示
double d2;
from_chars(buf, ptr, d2);
assert(d == d2);   // 保证 round-trip
```

`to_string`/`printf` 不保证 round-trip（精度不够时会丢精度）。

### 性能

`from_chars` 比 `strtol` 快约 2-5 倍，比 `iostream` 快 10-20 倍。`to_chars` 同理。

## HFT 关联

- **FIX 协议数值解析**：FIX 消息中的价格、数量用 `from_chars` 解析，比 `strtod` 快数倍，无 locale 开销。
- **日志数值格式化**：日志中的延迟、计数用 `to_chars` 写入缓冲，无 iostream 虚函数开销。
- **零分配**：`to_chars` 写入预分配栈缓冲，无堆分配，热路径可控。
- **round-trip 保证**：浮点参数序列化/反序列化用 `to_chars`/`from_chars`，不丢精度。
- **不 null-terminate 的优势**：直接写入消息缓冲中间，无需补 `\0` 再覆盖，少一次写入。
- **链式解析 FIX 字段**：`from_chars` 返回 ptr，连续解析 `|55=AAPL|44=150.25|` 各字段，无需 substr。

## 自测题

1. `to_chars`/`from_chars` 相比 `to_string`/`strtol` 的四个优势是什么？
2. `to_chars` 为什么不 null-terminate？调用方要注意什么？
3. `from_chars` 为什么不跳前导空白？这和 `strtol` 有什么区别？
4. round-trip 保证是什么意思？为什么 `printf` 不保证？
5. HFT FIX 协议解析如何用 `from_chars` 链式解析字段？
