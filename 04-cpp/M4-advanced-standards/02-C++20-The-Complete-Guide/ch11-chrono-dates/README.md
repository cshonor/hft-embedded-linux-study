# 第 11 章 日期与时区

**Dates and Timezones for <chrono>**

## 本章讲什么

C++20 给 `<chrono>` 加了**日历类型**（`year`/`month`/`day`）、**时区支持**（`time_zone`）、以及 `chrono` 字面量和 I/O。让时间处理从"秒/毫秒计数"升级到"日历+时区"。

## 要点

### 日历类型

```cpp
#include <chrono>
using namespace std::chrono;

// 日历字段
auto y = 2026y;        // year
auto m = March;         // month（枚举）
auto d = 15d;           // day

// 组合成日期
auto date = 2026y/March/15d;     // year_month_day
auto date2 = March/15/2026;      // 另一种顺序

// 日期运算
auto tomorrow = date + days{1};
auto next_month = date + months{1};
auto next_year = date + years{1};
```

### 时钟与时间点

```cpp
// 系统时钟（墙钟时间，带时区）
auto now = system_clock::now();
auto now_ms = floor<milliseconds>(now);

// 转日历
auto dp = floor<days>(now);
year_month_day ymd{dp};           // 日期部分
hh_mm_ss time_of_day{now - dp};   // 时间部分

// 格式化输出
std::format("{}", now);           // "2026-03-15 14:30:00.123 UTC"
std::format("{:%Y-%m-%d %H:%M:%S}", now);
```

### 时区

```cpp
// 获取时区
auto utc = locate_zone("UTC");
auto shanghai = locate_zone("Asia/Shanghai");
auto ny = locate_zone("America/New_York");

// 时区转换
auto sh_time = shanghai->to_local(now);
auto ny_time = ny->to_local(now);

// zoned_time
zoned_time zt{"Asia/Shanghai", now};
std::format("{}", zt);   // "2026-03-15 22:30:00 CST"
```

### 持续时间字面量

```cpp
using namespace std::chrono_literals;

auto ns = 100ns;     // nanoseconds
auto us = 50us;      // microseconds
auto ms = 10ms;      // milliseconds
auto s = 5s;         // seconds
auto min = 3min;     // minutes
auto h = 2h;         // hours
```

### 日历运算的正确性

```cpp
auto jan31 = January/31/2026;
auto feb = jan31 + months{1};   // 2月31日不存在 → 自动转为 2月28日
// chrono 日历运算处理月末/闰年，不会产生非法日期
```

### `clock_cast`（C++20）

```cpp
// 不同时钟间转换
auto sys_now = system_clock::now();
auto steady = clock_cast<steady_clock>(sys_now);
```

## HFT 关联

- **纳秒时间戳**：HFT 用 `nanoseconds` 精度时间戳，`chrono` 原生支持，`100ns` 字面量清晰。
- **墙钟 vs 单调时钟**：`system_clock`（墙钟，可回拨，用于日志时间）vs `steady_clock`（单调，用于延迟测量）。**延迟测量必须用 steady_clock**，否则系统时间回拨会导致负延迟。
- **时区转换**：跨市场（A股/美股）策略用 `zoned_time` 转换交易时间，不用手写 UTC+8 偏移。
- **日历运算做交易日**：`trading_day = today; if (friday) trading_day += days{3};` 计算下一交易日（简化版，完整逻辑要查日历表）。
- **`format` 时间戳**：日志用 `std::format("{:%H:%M:%S}", now)` 格式化时间，比 `strftime` 安全。
- **steady_clock 无时区**：`steady_clock` 没有日历/时区，只有"从某起点开始的纳秒"——适合延迟测量不适合日志时间。

## 自测题

1. `system_clock` 和 `steady_clock` 的区别？HFT 延迟测量用哪个？
2. C++20 日历类型如何处理"2月31日"这种非法日期？
3. 如何把 UTC 时间转成 Asia/Shanghai 时区？
4. `chrono` 字面量 `100ns`/`5s`/`2h` 的类型是什么？
5. HFT 为什么用 `steady_clock` 测延迟，`system_clock` 记日志时间？

## 代码自测

### Q1: 日历和时区
```cpp
using namespace std::chrono;

// 日历日期
year_month_day date = 2024y/January/15;
auto date2 = January/15/2024;  // 月/日/年
auto date3 = 2024y/1/15;       // 年/月/日

// 时钟
auto now = system_clock::now();
auto today = floor<days>(now);
year_month_day ymd{today};

// 时长
auto timeout = 500ms;  // 毫秒
auto interval = 1s + 500ms;  // 1500ms

// 时区（C++20）
zoned_time zt{"Asia/Shanghai", now};
std::cout << zt;  // 2024-01-15 19:30:00 CST
```
> C++20 chrono 相比 C++11/17 新增了什么？

<details>
<summary>答案与复习指引</summary>

**C++20 chrono 新增**：
1. **日历类型**：`year`/`month`/`day`/`year_month_day` 等
2. **时区支持**：`zoned_time`/`time_zone`，IANA 时区数据库
3. **日历运算**：`January + 2 months = March`、`2024y/2/29 + 1 year = 2025y/2/28`（自动处理闰年）
4. **格式化**：`std::format("{:%Y-%m-%d %H:%M:%S}", now)` 直接格式化时间

**C++11/17 chrono** 只有 `duration`（时长）和 `time_point`（时间点），没有日历/时区概念。

**HFT**：
- 时间戳用 `system_clock::now()` → `to_time_t`/`to_from_chars` 序列化
- 定时器用 `steady_clock`（单调时钟，不受系统时间调整影响）
- 日历用于日志/报告，不用于热路径

**复习：** → [chrono 日历](./README.md)
</details>
