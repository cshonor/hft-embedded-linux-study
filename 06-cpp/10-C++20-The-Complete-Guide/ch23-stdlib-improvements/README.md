# 第 23 章 标准库的小幅改进

**Small Improvements for the C++ Standard Library**

## 本章讲什么

C++20 标准库的杂项改进：`std::span`（第 9 章）、`std::format`（第 10 章）、`std::source_location`、`std::bit`、`std::numbers`、`std::ranges` 算法、`std::starts_with`/`ends_with`、`std::contains`（C++23）等。

## 要点

### `std::source_location`

```cpp
#include <source_location>

void log(const std::string& msg,
         const std::source_location& loc = std::source_location::current()) {
    std::cout << loc.file_name() << ':' << loc.line()
              << " [" << loc.function_name() << "] " << msg << '\n';
}

log("hello");   // 自动带文件名、行号、函数名
// 输出：main.cpp:10 [main] hello
```

替代 `__FILE__`/`__LINE__` 宏，类型安全。HFT 日志利器。

### `std::bit`（C++20）

```cpp
#include <bit>

std::popcount(0b1011u);    // 3（1 的个数）
std::countl_zero(0x00FFu); // 24（前导 0）
std::countr_zero(0xFF00u); // 8（尾随 0）
std::has_single_bit(16u);  // true（是 2 的幂）
std::bit_ceil(5u);         // 8（≥5 的最小 2 的幂）
std::bit_floor(5u);        // 4（≤5 的最大 2 的幂）
std::bit_width(5u);        // 3（表示 5 需要的位数）
std::rotl(0b0001, 2);      // 0b0100（循环左移）
```

位操作标准化，编译器用 `__builtin_popcount`/`POPCNT` 指令优化。

### `std::numbers`（C++20）

```cpp
#include <numbers>

std::numbers::pi;           // 3.14159265358979323846
std::numbers::e;            // 2.71828182845904523536
std::numbers::sqrt2;        // 1.41421356237309504880
std::numbers::ln2;          // 0.69314718055994530942
std::numbers::inv_pi;       // 1/π
// 比 #define PI 3.14159 精确、类型安全
```

### `std::starts_with` / `ends_with`

```cpp
// C++20：string/string_view 的 starts_with/ends_with
std::string s = "hello.cpp";
s.starts_with("hello");   // true
s.ends_with(".cpp");      // true

std::string_view sv = "prefix_data";
sv.starts_with("prefix"); // true
```

C++20 之前要写 `s.compare(0, 5, "hello") == 0` 或 `s.find("hello") == 0`，啰嗦。

### `std::ranges` 算法（第 6-8 章）

`std::ranges::sort`/`find`/`copy`/`transform` 等——接受范围而非迭代器对。

### `std::ssize`

```cpp
std::vector<int> v;
auto n = std::ssize(v);   // 有符号 size（ptrdiff_t），避免 size_t 溢出问题

for (auto i = std::ssize(v) - 1; i >= 0; --i) {  // 有符号，逆序循环安全
    use(v[i]);
}
```

C++20 前 `v.size()` 返回 `size_t`（无符号），`size_t - 1` 溢出成大正数，逆序循环出错。

### `std::is_sorted` / `std::is_partitioned`（ranges 版）

```cpp
std::ranges::is_sorted(v);
std::ranges::is_partitioned(v, pred);
```

## HFT 关联

- **`source_location` 日志**：`log("order filled")` 自动带文件/行号/函数名，无需手写 `__FILE__`。
- **`std::bit` 位操作**：`popcount` 算订单簿位数，`countl_zero` 做优先队列位查找——用 `POPCNT`/`LZCNT` 指令，纳秒级。
- **`std::numbers::pi`**：金融计算用 `pi` 常量，比 `#define` 精确、类型安全。
- **`starts_with`/`ends_with`**：合约符号判断 `sym.starts_with("IF")` 识别股指期货，替代 `find`。
- **`std::ssize` 逆序循环**：逆序遍历行情时用 `ssize` 避免无符号下溢。
- **`bit_ceil` 对齐**：`bit_ceil(capacity)` 向上取 2 的幂，分配器对齐用。

## 自测题

1. `std::source_location` 相比 `__FILE__`/`__LINE__` 宏有什么优势？
2. `std::popcount`/`countl_zero` 做什么？用什么指令优化？
3. `std::numbers::pi` 相比 `#define PI 3.14159` 好在哪？
4. `std::starts_with` 相比 C++17 的 `find` 写法有什么优势？
5. `std::ssize` 解决什么问题？为什么 `size_t` 逆序循环会出错？
