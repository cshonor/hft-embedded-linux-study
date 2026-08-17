# <chrono> 的 floor/round/ceil

## C++14 的局限

```cpp
using namespace std::chrono;

auto now = system_clock::now();  // time_point<system_clock, nanoseconds>

// C++14：duration_cast 截断
auto ms = duration_cast<milliseconds>(now.time_since_epoch());
// 纳秒 → 毫秒：截断，不四舍五入
// 1234567 ns → 1234 ms（截掉了 567 ns）
```

## C++17 新增 floor/round/ceil

```cpp
// floor：向下取整
auto f = floor<milliseconds>(now.time_since_epoch());
// 1234567 ns → 1234 ms

// round：四舍五入
auto r = round<milliseconds>(now.time_since_epoch());
// 1234567 ns → 1235 ms（四舍五入）

// ceil：向上取整
auto c = ceil<milliseconds>(now.time_since_epoch());
// 1234567 ns → 1235 ms
```

## 对 time_point 的操作

```cpp
// C++17 也支持 time_point 的 floor/round/ceil
auto tp = system_clock::now();
auto floored = floor<seconds>(tp);   // 向下取整到秒
auto rounded = round<seconds>(tp);   // 四舍五入到秒
auto ceiled  = ceil<seconds>(tp);    // 向上取整到秒

// 用途：对齐时间戳到整秒/整毫秒
auto minute_start = floor<minutes>(tp);  // 当前分钟的开始时刻
```

## 与 duration_cast 的区别

```cpp
// duration_cast：总是截断（向零取整）
duration_cast<milliseconds>(2500us);  // 2ms（截断）

// floor：向下取整
floor<milliseconds>(2500us);  // 2ms

// round：四舍五入
round<milliseconds>(2500us);  // 3ms（2.5 → 3）

// ceil：向上取整
ceil<milliseconds>(2500us);  // 3ms
```

## 实际应用

```cpp
// 1. 时间戳对齐
auto now = system_clock::now();
auto aligned_us = floor<microseconds>(now);  // 对齐到微秒

// 2. 找当前 bar 的起始时间
auto bar_start = floor<seconds>(now);  // 1 秒 bar 的起始

// 3. 超时计算：向上取整到毫秒
auto timeout = ceil<milliseconds>(some_duration);

// 4. HFT：纳秒精度时间戳
auto ts = steady_clock::now();
auto ns = floor<nanoseconds>(ts.time_since_epoch());
// 纳秒精度，用于延迟测量
```

## 自测题

1. C++17 chrono 的 `floor`/`round`/`ceil` 相比 `duration_cast` 有什么改进？
2. `round<milliseconds>(2500us)` 的结果是什么？`duration_cast` 呢？
3. `floor<seconds>(tp)` 对 `time_point` 做什么？
4. HFT 时间戳为什么要用 `floor<nanoseconds>`？
5. 找当前分钟起始时刻的写法？
