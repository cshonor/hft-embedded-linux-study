# from_chars 详解

## 基本用法

```cpp
#include <charconv>

// 字符串 → 整数
const char* s = "12345";
int val;
auto [ptr, ec] = std::from_chars(s, s + 5, val);
if (ec == std::errc{}) {
    // val == 12345, ptr 指向 s+5（全部解析完）
}

// 字符串 → 浮点
const char* f = "3.14159";
double d;
auto [p2, e2] = std::from_chars(f, f + 7, d);
// d == 3.14159
```

## 整数进制

```cpp
// 十进制（默认）
std::from_chars(s, s+n, val);       // base 10

// 十六进制
std::from_chars(s, s+n, val, 16);   // "ff" → 255

// 八进制
std::from_chars(s, s+n, val, 8);    // "377" → 255

// 二进制
std::from_chars(s, s+n, val, 2);    // "11111111" → 255
```

## 浮点格式

```cpp
using cf = std::chars_format;

// 自动检测格式
std::from_chars(s, s+n, d, cf::general);  // "3.14" / "1.5e2" 都行

// 只接受科学计数法
std::from_chars(s, s+n, d, cf::scientific);

// 只接受定点
std::from_chars(s, s+n, d, cf::fixed);

// 只接受十六进制浮点
std::from_chars(s, s+n, d, cf::hex);
```

## 不跳前导空白

```cpp
// from_chars 不跳空白！
const char* s = "  123";
int val;
auto [ptr, ec] = std::from_chars(s, s+5, val);
// ec == std::errc::invalid_argument（空格不是数字）

// 对比 strtol：跳前导空白
char* end;
long v = strtol("  123", &end, 10);  // v = 123, end 指向 '\0'
```

**设计理念**：`from_chars` 做最小化工作——不假设格式，不跳空白，不设 errno。调用方负责预处理。

## 链式解析

```cpp
// 解析 FIX 消息 "55=AAPL|44=150.25|38=100"
std::string_view msg = "55=AAPL|44=150.25|38=100";
const char* p = msg.begin();
const char* end = msg.end();

// 解析 tag
int tag;
auto [p2, ec] = std::from_chars(p, end, tag);
// tag = 55, p2 指向 '='

// 跳过 '='
p = p2 + 1;

// 找到下一个 '|'
const char* sep = std::find(p, end, '|');

// 解析值...
double price;
std::from_chars(p, sep, price);  // 解析 AAPL 失败（非数字）
// 对于字符串字段直接拷贝
```

## 返回值与错误处理

```cpp
struct from_chars_result {
    const char* ptr;    // 指向第一个未解析字符
    std::errc ec;       // 错误码
};

// 成功：ec == {}, ptr 指向未解析部分
// 无效参数：ec == errc::invalid_argument, ptr == first
// 溢出：ec == errc::result_out_of_range, ptr == last
```

## 自测题

1. `from_chars` 跳前导空白吗？和 `strtol` 有什么区别？
2. `from_chars` 的返回值 `ptr` 指向哪里？
3. 错误码 `invalid_argument` 和 `result_out_of_range` 分别什么意思？
4. 如何用 `from_chars` 链式解析 FIX 消息？
5. `from_chars` 支持 16 进制浮点吗？
