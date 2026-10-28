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

### 典型正当用途：条件编译日志开关（TheCherno #55 同款）

```cpp
#include <iostream>

#if PR_DEBUG == 1
#define LOG(x) std::cout << x << std::endl
#elif defined(PR_RELEASE)
#define LOG(x)                          // 空宏
#endif

int main() {
    LOG("Hello");
}
```

原理：`#if/#elif/#endif` 在**预处理阶段**判定，Release 下 `LOG(x)` 展开为空，连字符串字面量都不进二进制——这是宏**至今无法被 `inline`/`constexpr` 替代**的少数场景之一：

```
// clang -DPR_RELEASE -E 实测：main 里只剩一个分号
int main()
{
                ;
}
```

对比 `inline` 函数版日志：参数必须**先求值**再进入函数体，`if (level >= DEBUG)` 的运行期判断省不掉字符串格式化开销；宏是**源码级消失**，零开销。所以选型口诀里那句「`#include`/条件编译 → 宏（仅此领域）」是有实测依据的。

> 现代替代：spdlog / fmt 这类库用**编译期日志级别 + 宏包装**做同样的事（宏只负责"要不要调用"，真正的格式化交给类型安全函数），比裸 `std::cout` 更可控。C++20 起可用 `std::source_location` 自动补 `文件:行号`（见下文 [「C++20 之后 `#define` 还剩下哪些活路」](#c20-之后define-还剩下哪些活路)）。
>
> C 语言侧的同类体系（预定义符号、`#define`、`#if` 条件编译）见 [Pointers on C §14.3 条件编译](../../../../01-c-language/02-advanced-pointers-and-memory/ch14-preprocessor/14.3-条件编译.md)——两种语言的预处理指令几乎一致，差别主要在"该不该用宏"这一层。

### 这个写法的五个坑（实测 clang）

**坑 1：两个分支都不成立 → `LOG` 未定义，直接编译失败**

```cpp
// 没传任何 -D：#if PR_DEBUG == 1 → 0 == 1 假（未定义标识符在 #if 里按 0 处理）
// #elif defined(PR_RELEASE) → 也假 → LOG 从未定义
LOG("Hello");   // 实测 error: use of undeclared identifier 'LOG'
```

**坑 2：`-DPR_DEBUG=0` 同样编译失败**。`#elif` 只查 `PR_RELEASE`，`PR_DEBUG=0` 落在"两个都不成立"的空隙里。**修正**：补一个 `#else` 兜底（默认关日志），或改写为 `#if defined(PR_DEBUG) && PR_DEBUG`。

**坑 3：不要依赖 `-DPR_DEBUG`（不带 `=1`）**

clang/gcc 下 `-D name` 等价于 `-D name=1`，实测能过；但这不是所有工具链的保证（部分嵌入式编译器/自定义构建里 `-D` 可能定义为空），此时 `#if PR_DEBUG == 1` 变成 `#if == 1` → 预处理期语法错误。**统一显式写 `-DPR_DEBUG=1`**，CMake 里同理。

**坑 4：空宏会把实参整个吞掉——Debug/Release 行为不一致**

```cpp
int n = 0;
LOG(++n);              // Debug: 打印 1，n = 1；Release: n = 0（++n 从未执行）
```

实测：Debug 输出 `1 / n = 1`，Release 输出 `n = 0`。**铁律：日志宏的实参里绝不能有副作用**（`++i`、带状态的函数调用），否则 Release 版静默改变程序行为——这类 bug 只在发布后出现，最难查。

**坑 5：两个宏同时定义时 Debug 赢**。`#if/#elif` 只取第一个成立的分支，构建脚本必须保证二者互斥，否则"以为在 Release"实际却在打日志。

### 宏体本身的两个坑（与条件编译无关，但同款日志宏必踩）

**坑 A：`std::endl` 是「换行 + 强制 flush」，不是「更优雅的 `\n`」**

```cpp
std::cout << x << std::endl;   // 每次都 flush 一次缓冲区
std::cout << x << '\n';        // 只换行，交给缓冲区攒着批量写
```

实测（clang -O2，20 万行重定向到 `/dev/null`）：

| 写法 | 耗时 | 折合每行 |
|------|------|---------|
| `'\n'` | 47.0 ms | ≈ 0.24 µs |
| `std::endl` | **232.8 ms** | ≈ 1.16 µs（**慢约 5 倍**） |

日志宏一旦在热路径开着，`endl` 的 flush 就是延迟尖峰来源。**日志宏体一律用 `'\n'`，只在真正需要立刻落盘时（崩溃前、跨进程可见）才显式 flush。**

**坑 B：宏参数不能含「裸逗号」**

```cpp
#define LOG(x) std::cout << x << std::endl
LOG("a", "b");   // 实测 error: too many arguments provided to function-like macro invocation
```

宏的参数分隔只看逗号，**不认括号层级**（`LOG(std::pair<int,int>(1,2))` 同样会被切成两个参数）。要多参数就上变参宏：

```cpp
#define LOG(...) std::cout << __VA_ARGS__ << '\n'          // C++11 起
#define LOGF(fmt, ...) printf(fmt, __VA_ARGS__)            // 逗号在 __VA_ARGS__ 之后才安全
```

C23 / C++20 起可用 `__VA_OPT__` 处理「变参部分为空」的逗号（版本边界实测见 [Pointers on C §14.2.3 带参数的宏](../../../../01-c-language/02-advanced-pointers-and-memory/ch14-preprocessor/14.2-define/14.2.3-带参数的宏.md)）。

### 两处措辞校准（讲给别人的时候容易讲歪）

1. **宏不是「给代码起别名」**。"别名"暗示有实体、有作用域、有类型；宏只是**带参数的文本替换模板**，`LOG` 这个名字在编译后的符号表里根本不存在（前文 `nm` 证据）。讲的时候说"模板化的文本替换"比"别名"安全。
2. **空宏替换后不是「一行空白」，是留下一个空语句 `;`**。实测 `-DPR_RELEASE -E` 的输出是 `int main() { ; }`。这个 `;` 无害，但它确实是**一条语句**——所以 `if (c) LOG(x); else foo();` 展开成 `if (c) ; else foo();` 依然合法；反过来如果宏体写成两条语句又没包 `do{}while(0)`，`else` 就会悬空。

### 构建侧怎么定义

```bash
clang++ -std=c++17 -DPR_DEBUG=1   -o app main.cpp   # Debug
clang++ -std=c++17 -DPR_RELEASE=1 -o app main.cpp   # Release，零开销
```

```cmake
# 按构建类型自动切换，避免手改代码
target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:PR_DEBUG=1>
    $<$<CONFIG:Release>:PR_RELEASE=1>
)
```

VS：项目属性 → C/C++ → 预处理器 → 预处理器定义里加 `PR_DEBUG=1`。

### 同类机制：`assert` / `NDEBUG`

`assert(expr)` 是同一套思路的标准版：定义 `NDEBUG` 后 `assert` 展开成 `((void)0)`，**表达式完全不求值**——所以 `assert(++i)` 或 `assert(do_something())` 会和上面坑 4 一样，在 Release 下静默改变行为。判定表达式必须无副作用。

---

## HFT 视角

- 协议里的 **magic number**（消息类型、字段长度、tick size）用 **`constexpr` / `const`** 命名，崩溃栈和日志里能看到符号名，比裸数字好查。
- **热路径「像宏一样快」的小函数**（如 `max`、`clamp`）用 **`inline` / `constexpr` 模板**，避免函数式宏的多次求值坑（例如对计数器或指针自增传参）。
- 类内 **缓冲区大小、队列深度** 用 `static constexpr` 或 enum hack，便于封装且编译期定长数组。
- **日志**：Debug 侧用条件编译宏做到 Release 零开销（本节实测 `int main(){ ; }`），但宏体必须写 `'\n'` 而非 `std::endl`（实测慢约 5 倍，flush 是热路径尖峰源）；Release 空宏会吞掉实参，所以**日志实参一律无副作用**。生产级做法：日志级别编译期过滤 + 异步落盘（spdlog 等），热路径只做无锁 enqueue。

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

**题目 5：** 条件编译日志宏（本题全部有实测结果）

```cpp
#if PR_DEBUG == 1
#define LOG(x) std::cout << x << std::endl
#elif defined(PR_RELEASE)
#define LOG(x)
#endif

int n = 0;
LOG(++n);
std::cout << "n = " << n << std::endl;
```

> 问：① 不传任何 `-D` 直接编译会怎样？② `-DPR_DEBUG=0` 呢？③ `-DPR_DEBUG=1` 和 `-DPR_RELEASE` 两种构建下 `n` 分别打印多少？

<details>
<summary>参考答案</summary>

① **编译失败**：`#if PR_DEBUG == 1` 未定义时按 `0 == 1` 判定为假（`#if` 里未定义标识符一律当 0），`#elif defined(PR_RELEASE)` 也为假，`LOG` 从未定义 → 实测 `error: use of undeclared identifier 'LOG'`。

② 同样**编译失败**：`PR_DEBUG=0` 不满足 `== 1`，而 `#elif` 只查 `PR_RELEASE`，落在两个分支的空隙里。**修正**：加 `#else` 兜底（默认关日志）。

③ Debug 打印 `1` 然后 `n = 1`；Release 只打印 `n = 0`——空宏把实参 `++n` **整个丢弃**，自增从未发生。这是条件编译日志宏最阴的坑：**Release 下程序行为静默改变**。铁律：日志宏实参必须无副作用（不能 `++i`、不能放带状态的函数调用）。

补充：`assert(++i)` 同理——`NDEBUG` 下 `assert` 展开为 `((void)0)`，表达式不求值。

</details>
