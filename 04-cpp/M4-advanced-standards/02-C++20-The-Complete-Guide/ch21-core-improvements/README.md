# 第 21 章 核心语言的小幅改进

**Small Improvements for the Core Language**

## 本章讲什么

C++20 核心语言的杂项小改进：`consteval`/`constinit`（第 18 章）、聚合初始化用 `()`、`using enum`、`char8_t`、`<=>` 的细节、指针比较修正等。

## 要点

### `using enum`

```cpp
enum class Color { Red, Green, Blue };

// C++20：using enum 引入所有枚举值
void foo() {
    using enum Color;
    auto c = Red;   // 不用写 Color::Red
}
```

`using enum` 把枚举值引入当前作用域，减少 `EnumName::` 前缀。

### 聚合初始化用 `()`

```cpp
struct Point { int x, y; };

// C++17：只能用 {}
Point p1{1, 2};

// C++20：也可用 ()（允许 paren-init）
Point p2(1, 2);
```

C++20 允许聚合类型用小括号初始化（之前只能花括号）。

### `char8_t`

```cpp
// C++20：char8_t 是独立类型（C++17 是 char）
char8_t c = u8'A';               // UTF-8 字符
const char8_t* s = u8"你好";      // UTF-8 字符串

// 影响：u8"" 的类型从 const char[] 变成 const char8_t[]
// 老代码 const char* s = u8"..." 要改 const char8_t*
```

### 指针比较修正

```cpp
// C++20：比较不同对象的指针顺序（之前 UB）
int a, b;
bool less = &a < &b;   // C++20 定义为全序（之前 UB）
```

C++20 给所有指针定义了全序比较，`<`/`>` 不再是 UB。

### `consteval` 传播规则

`consteval` 函数只能被 `constexpr`/`consteval` 调用——保证 consteval 调用链全是编译期。

### `volatile` 弃用部分用法

C++20 弃用了 `volatile` 的一些用法（如复合赋值 `v += 1`），因为 volatile 的语义在多线程下不正确（应用 atomic）。但 volatile 本身没被弃用，只是部分用法。

## HFT 关联

- **`using enum` 简化枚举**：策略状态 `using enum State;` 在 switch 里直接写 `Ready` 而非 `State::Ready`。
- **`char8_t` UTF-8 字符串**：日志/监控字符串用 `u8""` 明确 UTF-8，跨平台不乱码。
- **指针全序比较**：用指针做 map key 时，`<` 比较在 C++20 有定义（之前是 UB）。
- **`consteval` 传播**：编译期参数生成链全 consteval，保证运行期零计算。
- **`volatile` 部分弃用提醒**：HFT 不该用 volatile 做线程间通信（用 atomic），C++20 弃用部分用法是正确方向。

## 自测题

1. `using enum` 的作用？解决了什么麻烦？
2. C++20 的 `char8_t` 相比 C++17 的 `char` 有什么变化？
3. C++20 对指针全序比较的修正是什么？之前为什么是问题？
4. `consteval` 函数的调用有什么限制？
5. C++20 弃用 volatile 的哪些用法？为什么？
