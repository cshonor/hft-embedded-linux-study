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

## 代码自测

### Q1: 核心语言改进
```cpp
// 1. 指定初始化（C++20）
struct Point { double x, y; };
Point p{.x = 1.0, .y = 2.0};  // 指定成员名

// 2. consteval：必须编译期求值
consteval int square(int x) { return x * x; }
constexpr int x = square(5);  // OK
// int y = square(rand());    // 编译错误：必须编译期

// 3. constinit：编译期初始化，但可运行时修改
constinit int global = square(5);  // 编译期初始化，但可修改

// 4. char8_t
const char8_t* u8str = u8"hello";  // C++20: char8_t 类型
```
> consteval 和 constexpr 的区别？constinit 解决什么问题？

<details>
<summary>答案与复习指引</summary>

**consteval vs constexpr**：
- `constexpr`：**可能**在编译期求值（也可以运行时调用）
- `consteval`：**必须**在编译期求值（运行时调用是编译错误）

| 关键字 | 编译期 | 运行时 | 用途 |
|--------|--------|--------|------|
| `constexpr` | ✅ 可 | ✅ 可 | 双用途函数 |
| `consteval` | ✅ 必须 | ❌ | 纯编译期函数 |
| `constinit` | ✅ 必须 | 可修改 | 防止静态初始化顺序问题 |

**constinit 解决的问题**：
```cpp
// 全局变量：动态初始化（运行时）→ 静态初始化顺序问题
int global = compute();  // compute() 在运行时调用，初始化顺序未定义

// constinit：保证编译期初始化
constinit int global = 42;  // 编译期初始化，无顺序问题
```

**指定初始化**（`.x = 1.0`）：C 风格，让代码更清晰。但 C++ 要求**按声明顺序**指定（C 允许乱序）。

**复习：** → [核心改进](./README.md)
</details>
