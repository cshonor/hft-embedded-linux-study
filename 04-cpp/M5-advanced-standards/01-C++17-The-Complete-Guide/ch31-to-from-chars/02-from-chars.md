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

<details>
<summary>参考答案</summary>

1. **不跳前导空白**——必须一开始就遇到数字（或 `-`），否则解析失败。
与 `strtol` 的主要区别：
   - `strtol` 会跳前导空白、`+`/`-` 都接受、受 locale 影响、`base == 0/16` 时识别 `0x` 前缀、失败靠 `errno`/返回值判断；
   - `from_chars` **不跳空白**、只接受 `-`（不接受 `+`，且只对有符号类型）、**无 locale**、不分配、不抛异常、base 为 16 时也**不识别 `0x`/`0X` 前缀**、错误通过 `std::errc` 返回。
所以 `"  123"`、`"+123"`、`"0x1f"`（base 16）对 `from_chars` 都是解析失败或只解析出部分。
2. 成功时 `ptr` 指向**第一个不匹配模式的字符**（即"未解析部分的起点"），若全部匹配则 `ptr == last`，`ec` 为值初始化。
失败时：`ec == errc::invalid_argument` → `ptr == first`（一个字符都没匹配上）；`ec == errc::result_out_of_range` → `ptr` 指向第一个不匹配模式的字符（模式匹配上了但值超出范围）。
两种情况 `value` 都**不被修改**，这是它作为"快速解析器"的重要保证之一。
3. `invalid_argument`：字符串**根本不匹配数字模式**（如 `"AAPL"`、空串、只有符号没有数字）→ 解析失败，`ptr == first`，`value` 不变。
`result_out_of_range`：模式匹配上了，但解析出的数值**超出目标类型可表示的范围**（溢出/下溢）→ `ec == result_out_of_range`，`value` 不变。
排查意义：前者通常是"字段类型判断错了"（比如拿字符串字段当数字解析），后者是"数值太大要换更宽的类型"。
4. 用返回的 `ptr` 作为下一次解析的起点，逐字段推进：
```cpp
std::string_view msg = "55=AAPL|44=150.25|38=100";
const char* p = msg.data();
const char* end = msg.data() + msg.size();

int tag;
auto r = std::from_chars(p, end, tag);   // tag = 55，r.ptr 指向 '='
if (r.ec != std::errc{}) { /* 处理错误 */ }
p = r.ptr + 1;                            // 跳过 '='

const char* sep = std::find(p, end, '|');
double price;
auto r2 = std::from_chars(p, sep, price); // 只在本字段范围内解析
if (r2.ec != std::errc{}) { /* 该字段不是数字，按字符串处理 */ }
```
要点：每次用 `ptr` 推进；用 `|` 分隔符先切出字段区间再传给 `from_chars`；对每个返回的 `ec` 都做检查（`from_chars` 不抛异常）。
5. 支持 `std::chars_format::hex` 这种格式，但有两条限制：
   - **不接受 `0x` / `0X` 前缀**——`"0x123"` 会被解析成数值 `0`，剩下 `"x123"` 未解析。
   - hex 格式下**不允许指数部分**（而 `scientific` 要求必须有指数、`fixed` 下指数不允许）。
```cpp
double d;
auto r = std::from_chars(first, last, d, std::chars_format::hex);
```
也就是说它能解析十六进制浮点**数字**，但不能解析带 `0x` 前缀的 C 风格字面量——前缀要自己先跳过。

</details>
