# 条款 2：尽量以 const、enum、inline 替换 #define

## 本节讲什么

本质建议：**用编译器取代预处理器**——`#define` 不属于 C++ 语言本身，没有类型、作用域和调试信息。单纯常量用 **`const`**（或 `constexpr`）；类内编译期整数常量必要时用 **enum hack**；形似函数的宏用 **`inline` 模板函数** 替代。

与 [条款 1](./item01-视C++为一个语言联邦.md) 的 **C 子语言**（预处理器）相对：能不用 `#define` 就不用，把名字交给编译器管理。

## 为什么不用 `#define`

| 问题 | 说明 |
|------|------|
| 不进符号表 | `#define ASPECT_RATIO 1.653` 在编译前被替换成数字；报错/调试器里只见 `1.653`，难追来源 |
| 盲目文本替换 | 可能产生多份字面量拷贝，目标代码膨胀；`const` 通常只保留一份 |
| 无类型检查 | 任何类型都能「塞进去」，错误延迟到运行或产生隐式转换 |
| 无作用域 | 没有 `private #define`；宏一旦定义，整文件（经 `#include` 后更大范围）都可见 |
| 函数式宏陷阱 | 参数多次求值、括号遗漏、副作用不可预测 |

---

## 1. 用 `const` 替换简单宏常量

```cpp
// ❌ 不好
#define ASPECT_RATIO 1.653

// ✅ 好
const double AspectRatio = 1.653;
// C++11 起更常见：constexpr double AspectRatio = 1.653;
```

### 常量指针（头文件中）

指针本身和所指对象都应 const，避免被改：

```cpp
const char* const authorName = "Scott Meyers";
// 更好：const std::string authorName = "Scott Meyers";
```

### 类内常量（封装）

`#define` 无法限制在类作用域内。要把常量**关进类里**且通常只要**一份**实体，用 **`static const` 成员**（C++11 起类内可初始化整型常量；见下 enum hack 的老编译器场景）：

```cpp
class GamePlayer {
private:
    static const int NumTurns = 5;   // 类专属常量，有封装
    int scores[NumTurns];            // 编译期需要常量时（视编译器/C++ 版本）
};
```

> 更现代的写法：`static constexpr int NumTurns = 5;` 或 C++17 **`inline static`** 数据成员。

---

## 2. enum hack（编译期整数常量）

老编译器可能不允许在类内为 `static const int` **就地赋初值**，却又要在编译期用到该值（如数组维度 `int scores[NumTurns];`）。可用 **the enum hack**：

```cpp
class GamePlayer {
private:
    enum { NumTurns = 5 };           // 行为上更像 #define
    int scores[NumTurns];
};
```

**优点：**

- 像 `#define` 一样：**不能合法取 enum 值的地址**（无法被当成指针/引用乱传）；
- 通常**不额外占对象存储**（编译期常量）；
- 是 **TMP（模板元编程）** 的基础技巧之一。

现代 C++ 中，优先 `static constexpr` / `inline static constexpr`；enum hack 仍值得知道，读老代码和 TMP 时会遇到。

---

## 3. 用 inline 模板函数替换函数式宏

### 宏的陷阱

```cpp
#define CALL_WITH_MAX(a, b) ((a) > (b) ? (a) : (b))

int a = 5, b = 0;
CALL_WITH_MAX(++a, b);   // a 可能被递增两次！结果依赖 a 与 b 的比较
```

即便给每个参数加括号，**副作用仍可能被求值多次**——宏不是真正的函数调用。

### 替代：inline 模板

```cpp
template<typename T>
inline const T& callWithMax(const T& a, const T& b) {
    return a > b ? a : b;
}

int a = 5, b = 0;
callWithMax(++a, b);     // ++a 只执行一次，行为可预测
```

| 对比 | 函数式 `#define` | `inline` 模板函数 |
|------|------------------|-------------------|
| 类型安全 | 无 | 有 |
| 参数求值 | 可能多次 | 各参数只求值一次 |
| 作用域 / 访问控制 | 无 | 有（可放 `namespace`、类 `private`） |
| 调试 / 符号 | 展开后难查 | 进符号表，可单步 |
| 效率 | 文本替换 | 内联后通常同等开销 |

C++11 起也可用 **`constexpr` 函数** 处理编译期可算的「宏函数」场景。

---

## 仍可能需要预处理器的情况

- **`#include`**、**`#ifdef`** 条件编译（头文件守卫、平台分支）——这些不是「用 const 替代常量宏」要消灭的对象。
- 与 [条款 1](./item01-视C++为一个语言联邦.md) 一致：预处理器属于 **C 子集工具**，该用时用，但**业务常量和小函数**别再用 `#define` 凑合。

---

## HFT 视角

- 协议里的 **magic number**（消息类型、字段长度、tick size）用 **`constexpr` / `const`** 命名，崩溃栈和日志里能看到符号名，比裸数字好查。
- **热路径「像宏一样快」的小函数**（如 `max`、`clamp`）用 **`inline` / `constexpr` 模板**，避免函数式宏的多次求值坑（例如对计数器或指针自增传参）。
- 类内 **缓冲区大小、队列深度** 用 `static constexpr` 或 enum hack，便于封装且编译期定长数组。

---

## 核心总结

1. **常量** → `const` / `constexpr`（头文件指针：`const char* const` 或 `std::string`）。
2. **类内编译期整数、老编译器或要禁止取地址** → enum hack（现代优先 `static constexpr`）。
3. **形似函数的宏** → **`inline` 模板函数**（或 `constexpr` 函数），保留效率与类型安全。
4. 一句话：**让编译器看见名字和类型，别让预处理器在暗处替换成裸数字和文本块。**

← [条款 1：语言联邦](./item01-视C++为一个语言联邦.md) | [条款 3：尽可能使用 const →](./item03-尽可能使用const.md)
