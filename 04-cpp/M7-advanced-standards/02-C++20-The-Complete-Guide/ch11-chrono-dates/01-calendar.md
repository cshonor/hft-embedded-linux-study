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

<details>
<summary>参考答案</summary>

1. 构建方式（两种都常用）：
```cpp
using namespace std::chrono;
year_month_day ymd1 = 2024y/1/15;                       // 字面量写法
year_month_day ymd2{year{2024}, month{1}, day{15}};     // 显式构造
year_month_day today{floor<days>(system_clock::now())};  // 从 sys_days 构造
```
支持的运算：
   - 与 `months` / `years` 相加减（`ymd + months{1}`、`ymd += years{1}`），会自动规范化；
   - 与 `sys_days`（即"天数精度的 time_point"）**双向转换**（`sys_days{today}`、`year_month_day{sd}`）；
   - `ok()` 判断日期是否有效（如 2 月 30 日为 false）；
   - 访问 `.year()` / `.month()` / `.day()`，以及相等/序比较。
注意：`year_month_day` **不能直接加 `days`**——要按天加减必须先转成 `sys_days` 再加，转回来。
2. `system_clock`：**墙钟时间**，表示"现在的日历时间"，可转 `time_t`、可做日历运算；但它**不单调**——会被 NTP 校时、手动改表、闰秒调整影响，时间甚至可能回退。
`steady_clock`：**单调时钟**，保证只增不减，不受系统时间调整影响，适合测量间隔。
HFT 延迟测量用 **`steady_clock`**：只有它才能保证"后一次测量一定不早于前一次"，测出的延迟不会因为校时而变成负数。（`high_resolution_clock` 名义上精度最高，但它通常只是 `steady_clock` 或 `system_clock` 的别名，不可依赖，需要单调性时直接写 `steady_clock`。）
3. 规则是"**只在不丢信息时才允许隐式转换**"：
   - 从**较粗**的 period 到**较细**的 period 可以隐式（一定精确）：`seconds → milliseconds → microseconds → nanoseconds`、`hours → minutes → seconds`。
   - 从**较细**到**较粗**（可能截断）必须显式 `duration_cast`：`milliseconds → seconds`、`microseconds → milliseconds` 都要 `duration_cast`。
   - 若目标 duration 的 **rep 是浮点类型**，则任何方向都允许隐式（浮点能表示分数）。
标准上的判据是：源 period 与目标 period 之比的分母必须为 1（或目标是浮点）。
注：笔记里"`microseconds us2 = ns;`（ns → us 隐式 OK）"的说法是**反的**——`nanoseconds → microseconds` 会截断，需要 `duration_cast`；反过来 `nanoseconds ns2 = us;` 才是合法的隐式转换。
4. 对 `time_point` 用 `floor` 向下取整到目标粒度（结果永远不晚于原时刻）：
```cpp
auto now          = system_clock::now();
auto second_start = floor<seconds>(now);       // 当前这一秒的起始
auto next_second  = second_start + seconds{1}; // 下一秒
```
要对齐到天/分钟，换成 `floor<days>` / `floor<minutes>` 即可；对**持续时间**用 `floor<seconds>(dur)`，若想"四舍五入"用 `round`，"向上取整"用 `ceil`，而 `duration_cast` 是向零截断。
5. 先把 `time_point` 降到"天"的精度，构造 `year_month_day`，再转成 `weekday`：
```cpp
using namespace std::chrono;
auto today  = year_month_day{floor<days>(system_clock::now())};
weekday wd  = weekday{sys_days{today}};     // 或 weekday{floor<days>(now)}
if (wd == Saturday || wd == Sunday) { /* 周末 */ }

// 也可取数值：0=周日 … 6=周六
int w = wd.c_encoding();
```
`floor<days>` 把 `time_point` 变成 `sys_days`（天精度的 time_point），`weekday` 再由它算出星期——这套类型保证了闰年、月份长度等日历规则都正确处理。

</details>
