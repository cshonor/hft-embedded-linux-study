# to_chars 详解

## 基本用法

```cpp
#include <charconv>

char buf[32];

// 整数 → 字符串
auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), 42);
// ptr 指向写入末尾（不包含 '\0'）
// ec == std::errc{} 表示成功
*ptr = '\0';  // 需要时手动补 null

// 浮点 → 字符串（默认最短表示）
auto [p2, e2] = std::to_chars(buf, buf + 32, 3.14159);
// 写入 "3.14159"
```

## 整数格式

```cpp
// 十进制（默认）
std::to_chars(buf, buf+32, 255);           // "255"

// 十六进制
std::to_chars(buf, buf+32, 255, 16);       // "ff"

// 八进制
std::to_chars(buf, buf+32, 255, 8);        // "377"

// 二进制
std::to_chars(buf, buf+32, 255, 2);        // "11111111"
```

## 浮点格式

```cpp
using cf = std::chars_format;

// 默认：最短 round-trip 表示
std::to_chars(buf, buf+32, 3.14, cf::general);
// "3.14"

// 科学计数法
std::to_chars(buf, buf+32, 3.14, cf::scientific);
// "3.140000e+00"

// 定点
std::to_chars(buf, buf+32, 3.14, cf::fixed);
// "3.140000"

// 十六进制浮点
std::to_chars(buf, buf+32, 3.14, cf::hex);
// "1.91eb8p+1"

// 指定精度
std::to_chars(buf, buf+32, 3.14159, cf::scientific, 2);
// "3.14e+00"
```

## 返回值

```cpp
struct to_chars_result {
    char* ptr;          // 指向写入末尾
    std::errc ec;       // 错误码
};

// 成功：ec == std::errc{}, ptr 指向末尾
// 缓冲区不够：ec == std::errc::value_too_large, ptr == last
```

## 与其他方案对比

```cpp
// std::to_string：有堆分配、有 locale
std::string s = std::to_string(42);  // 分配 string

// sprintf：有 locale、有缓冲区风险
char buf[32];
sprintf(buf, "%d", 42);

// snprintf：稍安全但仍有 locale
snprintf(buf, sizeof(buf), "%d", 42);

// to_chars：无分配、无 locale、无异常
auto [ptr, ec] = std::to_chars(buf, buf+32, 42);
```

## HFT 应用

```cpp
// FIX 消息序列化：零分配
char msg[256];
char* p = msg;
p += std::to_chars(p, msg+256, msg_type).ptr - p;
*p++ = '|';
p += std::to_chars(p, msg+256, price).ptr - p;
*p++ = '|';
p += std::to_chars(p, msg+256, qty).ptr - p;
// 直接写入消息缓冲，无 string 临时对象
```

## 自测题

1. `to_chars` 的返回值是什么？`ptr` 指向哪里？
2. `to_chars` 为什么不 null-terminate？
3. 浮点的"最短 round-trip 表示"是什么意思？
4. `to_chars` 支持哪些整数进制？
5. HFT FIX 消息序列化如何用 `to_chars` 实现零分配？
