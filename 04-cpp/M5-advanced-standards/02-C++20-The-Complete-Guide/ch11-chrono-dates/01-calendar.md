# 日历与时钟

## C++20 日历类型

```cpp
#include <chrono>
using namespace std::chrono;

// 年月日
year_month_day ymd = 2024y / August / 14d;
auto ymd2 = 2024y / 8 / 14;  // 同上

// 日期运算
auto today = year_month_day{floor<days>(system_clock::now())};
auto tomorrow = today + days{1};
auto next_week = today + weeks{1};
auto next_month = today + months{1};
auto next_year = today + years{1};

// 星期几
weekday wd = Sunday;
auto next_sunday = sys_days{today} + (wd - weekday{today});

// 构建
auto date = January / 1 / 2025y;  // 2025-01-01
auto date2 = 2025y / 1 / 15;       // 2025-01-15
```

## 时钟类型

```cpp
// system_clock：系统时钟（可调整，可能跳变）
auto now = system_clock::now();
// 适合：日志时间戳、跨进程时间

// steady_clock：稳定时钟（单调递增，不跳变）
auto start = steady_clock::now();
// ... do work ...
auto end = steady_clock::now();
auto elapsed = end - start;
// 适合：延迟测量、benchmark

// high_resolution_clock：通常别名于 steady_clock
```

## 持续时间

```cpp
// 各种精度
nanoseconds ns{1000};       // 1000 ns
microseconds us{1000};      // 1000 us
milliseconds ms{1000};      // 1000 ms
seconds s{60};              // 60 s
minutes min{60};            // 60 min
hours h{24};                // 24 h

// 隐式转换（小→大安全，大→小需显式）
microseconds us2 = ns;      // OK：ns → us
// nanoseconds ns2 = us;    // ❌ 精度丢失
nanoseconds ns2 = duration_cast<nanoseconds>(us);  // 显式

// C++17 floor/round/ceil
auto f = floor<microseconds>(ns);  // 向下取整
auto r = round<microseconds>(ns);  // 四舍五入
```

## HFT 时间处理

```cpp
// 延迟测量
auto t1 = steady_clock::now();
// ... process tick ...
auto t2 = steady_clock::now();
auto latency = duration_cast<nanoseconds>(t2 - t1);
std::cout << "Latency: " << latency.count() << " ns\n";

// 时间戳对齐到整秒
auto now = system_clock::now();
auto second_start = floor<seconds>(now);
auto next_second = second_start + seconds{1};

// 日历判断
auto today = year_month_day{floor<days>(now)};
if (!today.ok()) { /* 无效日期 */ }
auto weekday = weekday{std::sys_days{today}};
if (weekday == Saturday || weekday == Sunday) {
    // 周末
}
```

## 自测题

1. C++20 的 `year_month_day` 怎么构建？支持哪些运算？
2. `system_clock` 和 `steady_clock` 的区别？HFT 延迟测量用哪个？
3. 持续时间的隐式转换规则是什么？
4. 如何对齐时间戳到整秒？
5. 如何判断今天是星期几？
