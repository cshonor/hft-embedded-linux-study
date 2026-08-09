# 第 28 章 其他标准库小改进

**Other Small Library Features and Modifications**

## 本章讲什么

C++17 杂项标准库小改进：`<memory>` 的 `align`/`launder`、`<cmath>` 新函数、`<chrono>` 的 floor/round/ceil、`<utility>` 的 `as_const`、`<variant>`/`<optional>`/`<any>` 的辅助函数等。

## 要点

### `<memory>` 改进

```cpp
// std::align：在缓冲区内对齐
char buf[1024];
size_t space = sizeof(buf);
void* ptr = buf;
std::align(64, sizeof(Obj), ptr, space);   // 64 字节对齐

// std::launder（见第 32 章详述）：placement new 后的指针屏障
// std::uninitialized_default_construct_n 等
```

### `<cmath>` 新函数

```cpp
// C++17 新增数学函数
std::hypot(3, 4);       // 5.0（两参数版已有，C++17 加三参数）
std::hypot(3, 4, 12);   // 13.0
std::lerp(a, b, t);     // 线性插值：a + t*(b-a)（C++20 才正式，C++17 部分）
std::clamp(val, lo, hi); // 在 <algorithm>
```

### `<chrono>` 的 floor/round/ceil

```cpp
using namespace std::chrono;

auto now = system_clock::now();
auto floored = floor<seconds>(now);   // 向下取整到秒
auto rounded = round<seconds>(now);   // 四舍五入到秒
auto ceiled  = ceil<seconds>(now);    // 向上取整到秒
```

C++14 只有 `duration_cast`（截断），C++17 加了 floor/round/ceil 三个取整方式。

### `std::as_const`（见第 25 章）

### `variant`/`optional`/`any` 辅助

```cpp
// swap
std::optional<int> a = 1, b = 2;
std::swap(a, b);   // C++17 swap 特化

// hash 支持
std::unordered_map<std::variant<int, string>, int> m;  // C++17 可哈希
```

### `std::to_chars` / `from_chars`（见第 31 章）

C++17 最快的数值↔字符串转换，详见第 31 章。

### `<tuple>` 的改进

```cpp
// std::apply（见第 25 章）
// std::tuple_cat 合并多个 tuple
auto t = std::tuple_cat(std::make_tuple(1), std::make_tuple("hi"));
// t = tuple<int, const char*>(1, "hi")
```

### `std::scoped_allocator_adaptor` 改进

让嵌套容器（如 `vector<vector<int>>`）的分配器能传递到内层，C++17 修复了一些边缘情况。

## HFT 关联

- **`std::align` 64 字节对齐**：热路径数据结构用 `std::align` 在预分配缓冲中做 cache 行对齐，或直接 `alignas(64)`。
- **`chrono` floor/round/ceil**：时间戳处理用 `floor<microseconds>(ts)` 对齐到微秒，比 `duration_cast`（截断）更灵活。
- **`chrono` 精度**：HFT 用 `nanoseconds` 精度时间戳，C++17 chrono 完整支持。
- **`tuple_cat` 聚合参数**：策略参数多来源（配置 + 运行期），用 `tuple_cat` 合并后 `apply` 构造。
- **小改进不喧宾夺主**：这些小工具在 HFT 代码中零散使用，核心仍是 atomic/string_view/PMR 等大特性。

## 自测题

1. `std::align` 的作用是什么？HFT 如何用它做 cache 行对齐？
2. C++17 chrono 的 floor/round/ceil 相比 `duration_cast` 有什么改进？
3. `std::tuple_cat` 做什么？
4. `std::hypot` 三参数版的用途？
5. HFT 时间戳处理为什么用 `floor<nanoseconds>`？

## 代码自测

### Q1: 库小特性
```cpp
// scoped_lock（简化多锁）
std::mutex m1, m2;
std::scoped_lock lk(m1, m2);  // 一次锁两个，自动避免死锁

// to_chars / from_chars（零分配转换）
char buf[20];
auto res = std::to_chars(buf, buf+20, 3.14);
*res.ptr = '\0';

// uninitialized_default_construct
std::vector<Widget> v;
v.resize(100);  // 调用 100 次默认构造

// launder（解决 placement new 后的优化问题）
alignas(int) unsigned char buf[sizeof(int)];
new (buf) int(42);
int* p = std::launder(reinterpret_cast<int*>(buf));
```
> scoped_lock 和 lock_guard 的区别？to_chars 比 to_string 好在哪？

<details>
<summary>答案与复习指引</summary>

**scoped_lock vs lock_guard**：
- `lock_guard<Mutex>`：锁一个 mutex（C++11）
- `scoped_lock<Mutexes...>`：可变参数，锁多个 mutex（C++17），内部用 `std::lock` 避免死锁

单锁时 `scoped_lock` 和 `lock_guard` 等价。多锁时 `scoped_lock` 更安全简洁。

**to_chars vs to_string**：
- `to_string`：返回 `std::string`，有堆分配
- `to_chars`：写入预分配 buffer，无分配、无异常、locale 无关
- HFT 热路径数值转字符串用 `to_chars`（零分配、最快）

**复习：** → [库小特性](./README.md)
</details>
