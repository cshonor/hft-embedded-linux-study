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

### 实测：三个坑的精确触发（clang 23.1.0）

**坑 1：括号遗漏——`SQUARE(a+1)` 的经典 Quiz**

```cpp
#define SQUARE(X) X*X          // 参数没加括号
int a = 3, b = SQUARE(a+1);    // 展开成 a+1*a+1
// 实测 b = 7，不是 16！
```

**坑 2：双重求值——触发条件依赖三元分支，不是必现**

```cpp
#define MAX(A,B) ((A)>(B)?(A):(B))   // 括号全加齐了也躲不掉
int x = 5, y = 2;
int m = MAX(x++, y);   // 条件 (x++)>(y) 为真 → 命中 (x++) 分支
                       // 实测：m = 6（本想返回 5），事后 x = 7（加了两次！）

int x2 = 1, y2 = 2;
int m2 = MAX(x2++, y2); // 条件为假 → 只走 (y) 分支
                        // 实测：m2 = 2, x2 = 2（恰好只加一次——纯属侥幸）
```

同一个宏、同一个 `x++` 写法，**坑是否触发取决于运行期谁大谁小**——这是函数式宏最阴的地方：bug 和数据相关，测试用例选不好根本测不出来。模板版不存在这个问题：`mymax(x++, y)` 里 `x++` 是先求值一次的实参，结果永远确定。

**坑 3：优先级——宏体没括号时被外层运算符"吃掉"**

```cpp
#define BAD_MAX(A,B) A>B?A:B    // 宏体没加括号
BAD_MAX(3,2)+10                // 展开成 3>2?3:2+10 → ?: 优先级低于 +
                               // 实测 = 3（期望 13）
```

**调试证据（nm）**：模板函数 `mymax` 在符号表里有 `T __Z5mymaxIiET_S0_S0_`，崩溃栈、断点、单步都找得到名字；宏展开成表达式，符号表里什么都没有——`#define ASPECT_RATIO 1.653` 报错时只看到裸数字 `1.653`。

### 模板能做到、宏做不到的另外三件事

| 能力 | 宏 | 模板 |
|------|----|------|
| 做通用类 | ❌ 写不出 `std::vector<T>` 这种东西 | ✅ 类模板是 `vector`/`pair`/`tuple` 的本体 |
| 重载 / 重载决议 | ❌ 宏不参与重载 | ✅ 可与普通函数、其他模板重载决议（见 [C++Primer 16.3 §2.5](../../../../04-cpp/M0-entry-syntax/01-C++Primer/ch16-templates/16.3-模板实参推断.md)） |
| 递归展开 | ❌ 宏不能递归 | ✅ 模板可递归实例化——tuple 遍历的模板递归写法（见 [C++Primer 17.1 §6](../../../../04-cpp/M0-entry-syntax/01-C++Primer/ch17-special-library-facilities/17.1-tuple元组.md)） |

### 三者对比：宏 / 模板 / constexpr（同题实测）

`constexpr` 函数不是模板的替代品，两者解决不同维度的问题——**模板管"类型泛化"，constexpr 管"编译期求值"，宏两者都假装能干但都不安全**。同一个 `SQUARE` 三种写法实测（clang 23.1.0）：

```cpp
#define SQUARE_MACRO(X) ((X)*(X))                 // 预处理期：文本替换
template<typename T>
T square_tpl(T x) { return x * x; }               // 编译期实例化：泛型 + 运行期调用
constexpr int square_cx(int x) { return x * x; }  // 编译期可求值：结果能当常量
```

| 维度 | `#define` 宏 | 模板函数 | `constexpr` 函数 |
|------|-------------|----------|-------------------|
| 处理阶段 | 预处理期（纯文本替换） | 编译期（类型感知实例化） | 编译期（可求值）+ 运行期（也能跑） |
| 类型检查 | ❌ | ✅ | ✅ |
| 参数求值 | 可能多次（坑 2） | 恰好一次 | 恰好一次 |
| 类型泛化 | ❌ | ✅ `square_tpl(2.5)` 实测 6.25 | ✅（配模板，见下） |
| 结果当编译期常量（数组长度/`static_assert`/模板实参） | ✅（碰巧是文本） | ❌ | ✅ `constexpr int N = square_cx(4);` 实测 N=16 |
| 符号表（调试） | ❌ | ✅ `__Z10square_tplIdET_S0_` | ✅ `__Z9square_cxi` |
| C 语言可用 | ✅ | ❌ C++ 专属 | ❌ C23 前没有 |

**实测里两个关键证据**：

1. **模板函数的返回值不是常量表达式**：`int arr[square_tpl(4)];` 在 clang 默认下能编过——但那是 **VLA 扩展**在掩护（`-pedantic-errors` 实测报 `variable length arrays in C++ are a Clang extension`）；换 `constexpr` 版同开关下干净通过。想要"泛型 + 编译期常量"，两者要合体：

```cpp
template<typename T>
constexpr T square(T x) { return x * x; }   // constexpr 函数模板
constexpr int    a = square(4);             // 实测 16，可当数组长度
constexpr double d = square(2.5);           // 实测 6.25
static_assert(square(4) == 16);
```

2. **宏的坑在 constexpr 里全消失**：`square_cx(a+1)` 实测 = **16**（宏版无括号时 = 7）；`square_cx(a++)` 也只加一次——它真的是函数调用，参数先求值。

选型口诀：**常量 → `constexpr` 变量；形似函数的宏 → `constexpr` 函数（需要泛型就加模板）；类型泛化的类/函数 → 模板；`#include`/条件编译 → 宏（仅此领域）**。

constexpr 的完整体系（C++11 函数体限制 → C++14 放宽 → 字面类 → `if constexpr`）见 [C++Primer 2.4.3 constexpr 详解](../../../../04-cpp/M0-entry-syntax/01-C++Primer/ch02-variables-and-basic-types/2.4-const-qualifier/2.4.3-constexpr详解.md)。

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

## C++20 之后，`#define` 还剩下哪些活路

条款 2 写于 C++98 时代。25 年后，原来「不得不用宏」的地盘大半已被语言特性接管：

| 原本用宏做的事 | 现在的正解 | 从哪一代开始 |
|---|---|---|
| 常量 `#define N 100` | `constexpr auto N = 100;` | C++11 |
| 形似函数的宏 | `constexpr` / `inline` 函数模板 | C++98 / C++11 |
| `#define MAX(a,b) …` 的类型不安全 | `std::max` / `std::clamp`（`constexpr`） | C++17 |
| 捕获 `__FILE__` / `__LINE__` | **`std::source_location::current()`** | **C++20** |
| 头文件 include guard | 模块 `import`（顺带把宏关在模块里） | C++20 |
| 编译期字符串 / 反射 | `consteval` + `std::format` 等 | C++20/23 |
| 嵌入二进制资源 | `#embed`（C23 / C++26） | C++26 |

### 实测：`std::source_location` 是唯一能「干掉 `__FILE__`/`__LINE__` 宏」的东西

```cpp
/* 传统写法：只能是宏，因为 __FILE__/__LINE__ 必须在调用点展开 */
#define LOG_MACRO(msg) \
    std::fprintf(stdout, "%s:%d: %s\n", __FILE__, __LINE__, msg)

/* C++20：普通 inline 函数 + 默认实参，调用点自动填 file/line */
inline void log_cpp(const char *msg,
                    std::source_location loc = std::source_location::current()) {
    std::fprintf(stdout, "%s:%d: %s\n", loc.file_name(), loc.line(), msg);
}
```

CE 实测输出（**gcc 13.3 `-std=c++20` 与 clang 18.1.0 `-std=c++20` 逐字一致**）：

```text
/app/example.cpp:19: macro way
/app/example.cpp:20: source_location way
```

关键在于 `log_cpp` **不是宏**，却拿到了第 20 行这个**调用点**的位置——
默认实参在调用处求值，`current()` 被 `consteval` 化后由编译器就地填充。
这就把「位置捕获」从预处理期搬到了编译期，副作用（宏污染、`__LINE__` 在宏里被"吃掉"）全部消失。

### 但宏有四项是**结构上不可替代**的

| 能力 | 为什么替代不了 |
|---|---|
| `#if` / `#ifdef` 条件编译 | 只有预处理期能决定**哪些代码根本不存在**；`if constexpr` 要求两个分支都合法 |
| `##` 生成标识符 | 语言里没有任何机制凭空造出一个**新名字** |
| `#` 字符串化（`assert` 靠它打印表达式原文） | 反射之前，运行期拿不到「表达式的源码文本」 |
| X-Macro 表驱动 | 同一份数据在多处（枚举/字符串表/分发表）同步展开 |

⚠️ **C++20 modules 不导出宏**——这是刻意的：宏属于预处理期，模块边界在预处理之后。
所以「用模块消灭宏」只对 `#include` guard 有效，对上面四项无效。

---

## 核心总结

1. **常量** → `const` / `constexpr`（头文件指针：`const char* const` 或 `std::string`）。
2. **类内编译期整数、老编译器或要禁止取地址** → enum hack（现代优先 `static constexpr`）。
3. **形似函数的宏** → **`inline` 模板函数**（或 `constexpr` 函数），保留效率与类型安全。
4. 一句话：**让编译器看见名字和类型，别让预处理器在暗处替换成裸数字和文本块。**

← [条款 1：语言联邦](./item01-视C++为一个语言联邦.md) | [条款 3：尽可能使用 const →](./item03-尽可能使用const.md)

---

## 代码自测

**题目 1：** 下面两种定义常量的方式各有什么问题？
```cpp
// 方式A
#define ASPECT_RATIO 1.653
// 方式B
class GamePlayer {
    static const int NumTurns = 5;
    int scores[NumTurns];
};
```

<details>
<summary>参考答案</summary>

方式A：宏在预处理阶段替换，不进入符号表，调试看不到名字；没有作用域限制。
方式B：`static const int` 在类内给出初值是合法的，但某些编译器要求类外提供定义式（`const int GamePlayer::NumTurns;`）才能取地址。更稳妥的做法是 enum hack：`enum { NumTurns = 5 };`

</details>

**题目 2：** `#define SQUARE(X) X*X`，`int a = 3; int b = SQUARE(a+1);`，b 等于多少？为什么？改成 `#define SQUARE(X) ((X)*(X))` 后又是多少？

<details>
<summary>参考答案</summary>

b = **7**。展开成 `a+1*a+1` = 3+3+1——参数没加括号，乘法优先级把表达式切碎。加括号后 `((a+1)*(a+1))` = 16，但**括号只治这一种坑**：换成 `SQUARE(a++)` 依然是 `((a++)*(a++))` 双重自增（UB）——根治只能换 inline 模板函数，参数先求值一次。

</details>

**题目 3：** `#define MAX(A,B) ((A)>(B)?(A):(B))`（括号已加齐）。`MAX(x++, y)` 在 x=5、y=2 和 x=1、y=2 两种情况下，x 各自加了几次？

<details>
<summary>参考答案</summary>

x=5 时加**两次**（实测 m=6、事后 x=7）：条件 `(x++)>(y)` 为真，又命中 `(x++)` 分支，两处都执行。x=1 时只加**一次**（实测 m=2、x=2）：条件为假，只走 `(y)` 分支。同一个宏，坑是否触发**取决于运行期数据**——这就是函数式宏的测试盲区：用例没选到"参数赢"的分支，双求值 bug 就潜伏着。模板函数里实参永远先求值一次，与数据无关。

</details>

**题目 4：** 下面代码在 clang 默认开关下编译通过，为什么说它仍然是错的？

```cpp
template<typename T>
T square_tpl(T x) { return x * x; }
int main() {
    int arr[square_tpl(4)];   // 想让数组长度 = 16
    return arr[0];
}
```

<details>
<summary>参考答案</summary>

模板函数调用是**运行期值**，不是常量表达式——`int arr[运行期值]` 在标准 C++ 里非法。clang 默认放行是因为把它当成了 **VLA 扩展**（`-pedantic-errors` 实测报 `variable length arrays in C++ are a Clang extension`），GCC 同理。修正：给函数加 `constexpr`——`template<typename T> constexpr T square(T x)`，`square_tpl(4)` 就成了编译期常量，`arr` 是正经的定长数组（也才能用于 `static_assert`、模板实参）。MSVC 不支持 VLA 扩展，这段代码在 MSVC 直接编译失败——换编译器立刻暴露。

</details>
