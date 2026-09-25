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

<details>
<summary>参考答案</summary>

1. `duration_cast` 只有一种取整方向：**向零截断**（truncation toward zero）。
C++17 新增的 `floor` / `ceil` / `round` 提供了明确的向下取整、向上取整、四舍五入（ties away from zero），并且它们同时有 `duration` 和 `time_point` 两个版本，不必再手写 `tp - tp % period` 之类的对齐表达式。
对时间跨度/时间戳做「对齐到整秒」「超时向上取整」这类操作时，语义不再需要靠注释解释。
2. `round<milliseconds>(2500us)` 得到 **3ms**（2.5 按四舍五入、平局时远离零的方向取到 3）。
`duration_cast<milliseconds>(2500us)` 得到 **2ms**（向零截断）。
两者在正数上就有差别，在负数上差别更明显：`duration_cast` 向零截断，`floor` 向负无穷取整，`ceil` 向正无穷取整——涉及 epoch 之前的 `time_point` 时尤其要注意别用错。
3. 它把 `time_point` 向下取整到目标 duration 粒度：**返回不大于原 `time_point`、且能用该 duration 表示的最大 `time_point`**。
```cpp
auto tp      = system_clock::now();
auto floored = floor<seconds>(tp);   // 当前这一秒的开始
```
这正好是「bar 起始时刻」「分钟起点」需要的语义——结果永远不晚于原时刻，不会因为取整把时间戳推到未来。
4. 因为延迟测量需要**整数计数**且**语义确定**：`floor<nanoseconds>` 把时钟原生的 duration 转成纳秒整数，只舍不入，得到的计数可直接相减、排序、打日志，不会因为取整方向不同而产生「测量出的延迟是负数」这类错觉。
```cpp
auto ts = steady_clock::now();
auto ns = floor<nanoseconds>(ts.time_since_epoch());
```
注意时钟的真实分辨率由实现决定（是否真有纳秒级精度需查 `steady_clock::period`），`floor` 只保证取整方向，不提升精度。
5. 对 `time_point` 用 `floor` 向下取整到分钟粒度即可：
```cpp
auto now         = system_clock::now();
auto minute_start = floor<minutes>(now);   // 当前分钟的开始时刻
auto bar_start    = floor<seconds>(now);   // 1 秒 bar 的起始（同理）
```
若要先把时间戳对齐到微秒再取分钟，可以写 `floor<minutes>(floor<microseconds>(now))`。

</details>
