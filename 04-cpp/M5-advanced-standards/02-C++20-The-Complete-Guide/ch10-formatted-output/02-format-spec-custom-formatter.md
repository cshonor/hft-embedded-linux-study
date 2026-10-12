# 格式说明符全表与自定义 formatter

## 格式说明符结构

```
{[arg-id][:[fill]align][sign][#][0][width][.precision][type]}
```

按位置逐个拆：

```cpp
#include <format>

std::format("{:>10}", 42);       // 右对齐宽 10： "        42"
std::format("{:*^11}", 42);      // '*' 填充居中宽 11："****42*****"
std::format("{:+} {: }", 5, 5);  // 显式正号/空格位："+5  5"
std::format("{:#x} {:#b}", 255, 5); // 带前缀："0xff 0b101"
std::format("{:08.3f}", 3.14159);   // 0 填充宽 8 精度 3："0003.142"
std::format("{:.2e}", 12345.678);   // 科学计数："1.23e+04"
std::format("{:*>+12,.2f}", 1234.5);// 组合：  "***+1,234.50"
```

## type 速查（按值类型）

| 值 | type | 效果 | HFT 典型用途 |
|---|---|---|---|
| 整数 | `d`/`x`/`X`/`b`/`o`/`c` | 十/十六/二/八/字符 | symbol_id、flag 位、错误码 |
| 浮点 | `f`/`F`/`e`/`E`/`g`/`a` | 定点/科学/自动/hex-float | 价格 `{:.2f}`、延迟 `{:.0f}` ns |
| 字符串 | `s` | 字符串（可配宽度/精度=截断） | 日志列对齐 |
| 指针 | `p` | 地址 | 调试内存池 |
| 时间 | `%H:%M:%S` 等 chrono 格式 | 见下 | 行情时间戳 |

## chrono 直接格式化（不用再转 tm）

```cpp
#include <chrono>
#include <format>

auto now = std::chrono::system_clock::now();
std::format("{:%H:%M:%S}", now);
// ⚠ 实测（libc++）：输出 "12:41:14.940992"——system_clock 精度是微秒级，
//   %S 会自动带上小数部分！要整秒先截断：
std::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::seconds>(now));
//   → "12:41:14" ✅

std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::floor<std::chrono::seconds>(now));
```

> **Mac/cdev 工具链实测坑**：clang 23 + libc++ 编译 `std::format` 的**浮点**输出时报 `'to_chars' is unavailable: introduced in macOS 13.3`。原因：conda 工具链默认 deployment target 太老。解法：编译加 `-mmacosx-version-min=13.3`（整数/字符串不受影响，浮点格式化和 chrono 需要）。

## 自定义 formatter：让领域类型直接进 format

HFT 日志里到处都是 `Order`/`Tick`，逐个字段拼字符串太蠢。特化 `std::formatter`：

```cpp
#include <format>

struct Order {
    int32_t symbol_id;
    double  price;
    int64_t qty;
    char    side;   // 'B' / 'S'
};

template <>
struct std::formatter<Order> {
    // parse：解析 {} 里的格式说明（这里只支持 {:.2f} 精度的简版）
    int precision = 2;

    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == '.') {
            ++it;
            precision = *it++ - '0';   // 简化版：只读一位
        }
        if (it != ctx.end() && *it == 'f') ++it;
        return it;   // 返回指向 '}' 的迭代器
    }

    // format：真正输出
    auto format(const Order& o, std::format_context& ctx) const {
        return std::format_to(ctx.out(),
            "{}{} @ {:.{}f} x {}",
            o.side, o.symbol_id, o.price, precision, o.qty);
    }
};

// 用起来：
Order o{700, 182.345, 100, 'B'};
std::format("{}", o);       // "B700 @ 182.34 x 100"（见下注）
std::format("{:.4f}", o);   // "B700 @ 182.3450 x 100"
```

> **实测（clang 23 + libc++）**：默认精度 2 时输出 `182.34` 而非直觉的 `182.35`——`182.345` 的二进制浮点实际是 `182.34499999...`，四舍五入后向下。**打印价格永远显式写精度，且对"半点进位"敏感的场景要用定点整数存价格**（分/厘），double 只用于传输。

两步合同：`parse` 吃格式说明、返回指向 `}` 的迭代器；`format` 用 `format_to(ctx.out(), ...)` 输出。

## format_to_n / format_to：写进自己的缓冲区

低延迟日志不能每次格式化都堆分配。`format_to` 系列把输出写进**你提供的 buffer**：

```cpp
char buf[256];

// format_to_n：最多写 n 字符，返回 {写入位置, 应写总长度}
auto [out, size] = std::format_to_n(buf, sizeof(buf),
    "[{}] B{} @ {:.2f}", "12:00:01", 700, 182.345);
// size 可能 > sizeof(buf)（被截断时），用它判断截断

// 配合内存日志环（ring buffer log）：
// 热路径只 format_to_n 到无锁环形缓冲区，真正的 IO 由后台线程做
```

这就是"前端异步日志"的基本件——陈硕 muduo 日志库的核心思想，C++20 之后格式化这一步有标准库兜底了。

## vformat / make_format_args：格式串运行期才确定

```cpp
// 格式串不是编译期常量（如从配置读）时：
std::string tpl = load_log_template();
std::vformat(tpl, std::make_format_args(700, 182.345));

// 编译期检查版本（C++26 才有编译期强制的 format_string，C++20 是约定）：
// 常量格式串用 format()，编译器能查参数个数/类型对不对
std::format("{0} {1}", 700, 182.345);   // 编译期知道要 2 个参数
```

## format vs iostream vs printf（HFT 视角）

| 维度 | printf | iostream | std::format |
|---|---|---|---|
| 类型安全 | ❌ 格式串和参数错位即 UB | ✅ | ✅（编译期查格式串） |
| 性能 | 中（每次解析格式串） | 差（多次虚调用+locale） | 好（编译期解析格式串） |
| 可扩展 | ❌ | 麻烦 | ✅ formatter 特化 |
| 写进自有 buffer | snprintf（易错） | 不行 | ✅ format_to_n |
| 浮点 | %g 默认丢精度 | 默认 6 位 | shortest round-trip |

## 自测题

1. `{:08.3f}` 每一位的含义？`{:*^+12,.2f}` 能读出来吗？
2. 自定义 formatter 的两个成员函数合同是什么？`parse` 返回什么？
3. 为什么低延迟日志要用 `format_to_n` 到环形缓冲区，而不是直接 `std::format` 返回 string？
4. `format_to_n` 返回的 size 大于缓冲区长度意味着什么？
5. std::format 对比 printf 在浮点输出上的默认行为差异？为什么这正好适合打印价格？
