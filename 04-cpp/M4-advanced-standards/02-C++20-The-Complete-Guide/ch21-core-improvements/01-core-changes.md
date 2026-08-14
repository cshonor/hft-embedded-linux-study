# 核心语言改进

## 指定初始化

```cpp
struct Point { int x; int y; int z = 0; };

// C++20：指定初始化（按声明顺序）
Point p = {.x = 1, .y = 2};
Point p2 = {.x = 1, .y = 2, .z = 3};

// 不能跳过或重排
// Point p3 = {.y = 2, .x = 1};  // ❌ 顺序不对
// Point p4 = {.x = 1, .z = 3};  // ❌ 跳过 y

// 优势：明确每个值的含义
struct Config {
    int max_orders;
    double risk_limit;
    bool enable_logging;
};
Config cfg = {.max_orders = 100, .risk_limit = 0.05, .enable_logging = true};
// 比 Config{100, 0.05, true} 清晰
```

## consteval 和 constinit

```cpp
// 详见第 18 章
consteval int compile_only(int n) { return n * 2; }
constinit int x = compile_only(21);
```

## 运算符优先级调整

```cpp
// C++20： spaceship 优先级在 < 和 == 之间
// a <=> b < 0  →  (a <=> b) < 0  // 正确
```

## 更宽松的 constexpr

```cpp
// 详见第 18 章
// constexpr 可用循环、try/catch、std::vector 等
```

## using enum

```cpp
enum class Color { Red, Green, Blue };

// C++20：using enum 引入所有枚举值
void foo() {
    using enum Color;
    auto c = Red;   // 不用写 Color::Red
    auto c2 = Blue;
}
```

## 字符集改进

```cpp
// C++20：char8_t 类型
char8_t c = u8'A';  // UTF-8 字符
const char8_t* s = u8"hello";  // UTF-8 字符串

// C++17：u8 返回 const char*
// C++20：u8 返回 const char8_t*
```

## 自测题

1. 指定初始化 `{.x = 1, .y = 2}` 的规则是什么？能跳过成员吗？
2. `using enum` 做什么？
3. `char8_t` 是 C++20 新增的类型吗？为什么需要？
4. C++20 的 spaceship 运算符优先级在哪里？
5. 指定初始化相比聚合初始化 `{1, 2}` 有什么优势？
