# 第 10 章 格式化输出

**Formatted Output**

## 本章讲什么

C++20 的 `std::format` 是 Python `format`/`f-string` 风格的类型安全格式化，替代 `printf`（不安全）和 `iostream`（慢）。C++23 加 `std::print` 直接输出。

## 要点

### 基本用法

```cpp
#include <format>

// 占位符 {}
std::string s = std::format("Hello, {}! You are {}.", name, age);

// 位置参数
std::format("{1} before {0}", "A", "B");  // "B before A"

// 格式说明符
std::format("{:d}", 42);          // 整数十进制
std::format("{:x}", 255);         // 16 进制 "ff"
std::format("{:08x}", 255);       // 8 位补零 "000000ff"
std::format("{:.2f}", 3.14159);   // 2 位小数 "3.14"
std::format("{:>10}", "hi");      // 右对齐宽 10
std::format("{:<10}", "hi");      // 左对齐
std::format("{:^10}", "hi");      // 居中
std::format("{:*>10}", "hi");     // 右对齐补 *
```

### 相比 printf / iostream

| 方案 | 类型安全 | 性能 | 可读性 | 扩展性 |
|------|----------|------|--------|--------|
| `printf` | 不安全（格式串和参数不匹配） | 快 | 一般 | 不可扩展自定义类型 |
| `iostream` | 安全 | 慢（虚函数/locale） | 啰嗦 | 可扩展但麻烦 |
| `std::format` | 安全（编译期检查） | 快（接近 printf） | 好（Python 风格） | 可扩展（formatter 特化） |

### 自定义类型格式化

```cpp
template <>
struct std::formatter<Tick> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Tick& t, format_context& ctx) const {
        return std::format_to(ctx.out(), "Tick{{px={}, qty={}}}", t.px, t.qty);
    }
};

Tick t{150.25, 100};
std::format("{}", t);  // "Tick{px=150.25, qty=100}"
```

### `std::format_to`：写入迭代器

```cpp
std::string buf;
std::format_to(std::back_inserter(buf), "{} + {} = {}", 1, 2, 3);

// 写入预分配缓冲（无堆分配）
char buf[256];
auto [ptr, ec] = std::format_to_n(buf, sizeof(buf), "{}", value);
```

### `std::print`（C++23）

```cpp
std::print("Hello, {}!\n", name);     // 直接输出，无 endl 开销
std::println("{} + {} = {}", 1, 2, 3); // 自带换行
```

C++20 只有 `format`（返回 string），C++23 才有 `print`（直接输出）。

### 编译期格式串检查

```cpp
std::format("{:d}", "hello");  // 编译错！d 格式不适用于字符串
std::format("{:x}", 3.14);     // 编译错！x 格式不适用于浮点
```

格式串是 `consteval` 检查的，编译期捕获格式与类型不匹配。

## HFT 关联

- **日志格式化**：`std::format("ts={} sym={} px={:.4f} qty={}", ts, sym, px, qty)` 比 iostream 快、比 printf 安全。
- **`format_to` 写预分配缓冲**：热路径日志用 `format_to_n(buf, sizeof(buf), ...)` 写栈缓冲，无堆分配。
- **自定义 Tick 格式化**：`formatter<Tick>` 特化让日志直接 `{}` 输出 Tick，无需手写字段拼接。
- **编译期格式检查**：格式串类型错误编译期捕获，比 printf 的运行期崩溃安全。
- **C++23 `print` 待升级**：C++20 项目仍用 `format` + `fputs`/`write` 输出，C++23 升级后直接 `print`。
- **比 iostream 快 5-10 倍**：无虚函数、无 locale、无 sentry 对象，热路径日志性能关键。

## 自测题

1. `std::format` 相比 printf 和 iostream 的三个优势？
2. 格式说明符 `{:08x}` / `{:.2f}` / `{:>10}` 分别什么意思？
3. 如何为自定义类型 Tick 实现 `formatter` 特化？
4. `format_to` 和 `format` 的区别？`format_to_n` 的用途？
5. HFT 热路径日志如何用 `format_to_n` 避免堆分配？
