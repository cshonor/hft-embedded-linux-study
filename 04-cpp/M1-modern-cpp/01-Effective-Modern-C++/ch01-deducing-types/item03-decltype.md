# Item 3：理解 decltype

> 第 1 章 类型推导 · Item 3 · 上一节：[Item 2 auto 推导](item02-auto-type-deduction.md) · 下一节：[Item 4 查看推导结果](item04-viewing-deduced-types.md)

## 为什么要学这个（先建立直觉）

`auto` 解决的是"我不想写类型"；`decltype` 解决的是另一个问题——"**这个表达式的精确类型是什么**"。`auto` 推导时会主动帮你剥掉引用和顶层 const（按值语义），而 `decltype` 是个诚实的工具——你给它什么，它就原样告诉你什么，不剥、不改、不加工。

这种"诚实"在写泛型转发代码时是刚需：转发函数的返回类型必须和被转发者**一模一样**，引用一个都不能丢。`auto` 做不到（它一定剥），`decltype(auto)` 就是为这个场景造的。读懂标准库的 `std::forward`、`decltype(auto)` 包装器、尾置返回类型 `auto f() -> decltype(expr)`，全靠这一节。

---

## 这节讲什么

1. `decltype` 的两条分支规则：**名字** vs **表达式**，以及"加括号变引用"陷阱；
2. `decltype(auto)`（C++14）与 `auto` 返回类型推导的本质区别——一个保引用、一个剥引用，`return (x);` 的悬垂现场；
3. **C++11 尾置返回类型** `auto f(It b, It e) -> decltype(*b)` 的经典用例，以及 C++14 `decltype(auto)` 如何简化它（含版本差异实测）；
4. `decltype` 与 `std::declval` 的组合用法：不构造对象就查询成员调用结果、让不可行候选安静退出重载集。

---

## 核心规则：两条分支

| 你给的参数 | decltype 返回 | 例子 |
|---|---|---|
| **未加括号的变量名/成员访问** | 该实体的**声明类型**，原样保留 | `decltype(x)` → `x` 声明成什么就是什么 |
| **其他一切表达式** | 按值类别：左值→`T&`，右值→`T`（纯右值）/ `T&&`（将亡值） | `decltype(x + 1)` → `int`；`decltype((x))` → `int&` |

clang 23.1.0 实测（全部 `static_assert` 通过）：

```cpp
#include <type_traits>
using std::is_same_v;
int& getRef() { static int g = 7; return g; }
int  getVal() { return 42; }

int main() {
    int x = 42;
    const int& cx = x;
    static_assert(is_same_v<decltype(x),  int>);         // 分支一：名字 → 声明类型
    static_assert(is_same_v<decltype(cx), const int&>);  //   引用和 const 都不剥
    static_assert(is_same_v<decltype(x + 1), int>);      // 分支二：右值表达式 → T
    static_assert(is_same_v<decltype(getRef()), int&>);  //   返回左值引用 → int&
    static_assert(is_same_v<decltype(getVal()), int>);   //   返回右值 → int
    static_assert(is_same_v<decltype((x)), int&>);       // ← 加括号陷阱
    return 0;
}
```

`decltype` 是**完全不加工**的那一个：`auto` 永远走"按值"那套（除非你自己写 `&`），`decltype` 则 `const int&` 进去就 `const int&` 出来。

### 加括号陷阱的机制

`decltype(x) a = 1;` 是 `int`，而 `decltype((x)) b = x;` 是 `int&`——`x` 是"未加括号的变量名"走分支一，`(x)` 是**表达式**走分支二，左值给 `int&`。`decltype(auto)` 用在变量声明上同样踩坑（实测：`decltype(auto) a = x;` 是 `int`，`decltype(auto) b = (x);` 是 `int&`）。注意 `decltype(cx)` 与 `decltype((cx))` 都是 `const int&`，但机制不同（声明类型 vs 左值表达式）——巧合掩盖机制，别记特例，记两条分支。

---

## decltype(auto)：为转发而生

C++14 起 `decltype(auto)` 可做占位符：语法位置上是 `auto`，推导规则用 `decltype`。这解决了 `auto` 返回类型推导的硬伤——**`auto` 会剥掉引用**：

```cpp
auto badWrapper()            { return getRef(); }   // 返回 int：引用被剥，拷了一份
decltype(auto) goodWrapper() { return getRef(); }   // 返回 int&：原样转发

static_assert(is_same_v<decltype(badWrapper()),  int>);
static_assert(is_same_v<decltype(goodWrapper()), int&>);
```

剥引用不只是"多一次拷贝"——它直接改变接口能力：`badWrapper() = 100;` 报 `error: expression is not assignable`（返回的是右值），而 `goodWrapper() = 100;` 能写穿到原对象（实测 `getRef()` 输出 `7` → `100`）。

转发容器元素（`c[i]` 返回引用）或成员引用的包装器，这个差别就是要命和不要命的区别；泛型转发配合 `std::forward` 的标准写法是 `template<class T> decltype(auto) wrapper(T&& x) { return std::forward<T>(x); }`——`forward` 结果是 `T&` 就给 `T&`、是 `T&&` 就给 `T&&`。

### 陷阱：`return (x);` 悬垂

`decltype(auto)` 用 `decltype` 规则推导 `return` 表达式——加括号陷阱在这里重现，后果是 UB：

```cpp
decltype(auto) f() { int x = 42; return (x); }   // (x) 是左值 → 返回 int& → 悬垂
```

clang 23.1.0 实测警告原文（**不加 `-Wall` 也报**，`-Wreturn-stack-address` 默认开）：

```
warning: reference to stack memory associated with local variable 'x' returned [-Wreturn-stack-address]
    1 | decltype(auto) f1() { int x = 42; return (x); }
      |                                           ^
```

`return x;`（不加括号）推导为 `int`，安全。**写 `decltype(auto)` 函数时把 `return` 的表达式当地雷排**：永远别给返回的变量名加括号。

---

## C++11 尾置返回类型：decltype 的经典战场

C++11 里返回类型写在参数列表**之前**，而模板常常要"根据参数推导返回类型"——参数这时还没声明。尾置返回类型就是把返回类型挪到参数之后：

```cpp
template<class It>
auto firstOf(It b, It e) -> decltype(*b) { return *b; }   // b 在这里才可见
```

写成前置形式直接失败（clang 23.1.0 实测报错原文）：

```
error: use of undeclared identifier 'b'
    4 | decltype(*b) firstOf(It b, It e) { return *b; }
      |           ^
```

### 三种写法对照（实测）

| 写法 | 最低标准 | `vector<int>` 上的返回类型 | 特点 |
|------|---------|--------------------------|------|
| `auto f(It b, It e) -> decltype(*b)` | C++11 | `int&` | C++11 唯一能表达的写法；签名冗长 |
| `auto f(It b, It e)` | C++14 | `int`（**剥引用**） | 最简洁，但语义被改成拷贝 |
| `decltype(auto) f(It b, It e)` | C++14 | `int&` | 简洁 + 保引用，C++14 起首选 |

```cpp
std::vector<int> v{1, 2, 3};
static_assert(is_same_v<decltype(firstOfCxx11(v.begin(), v.end())), int&>);
static_assert(is_same_v<decltype(firstOfCxx14(v.begin(), v.end())), int&>);
static_assert(is_same_v<decltype(firstOfAuto(v.begin(), v.end())),  int>);
firstOfCxx14(v.begin(), v.end()) = 99;   // 写穿；实测随后 v[0] == 99
```

把 C++14 写法丢给 `-std=c++11`，实测：

```
error: 'auto' return without trailing return type; deduced return types are a C++14 extension
warning: 'decltype(auto)' type specifier is a C++14 extension [-Wc++14-extensions]
error: deduced return types are a C++14 extension
```

结论：**C++11 里尾置返回类型不是风格选择，是唯一出路**；升到 C++14 后 `decltype(auto)` 既短又不丢引用。

---

## decltype + std::declval：不构造对象就查询类型

`std::declval<T>()`（`<utility>`）是**只有声明、没有定义**的函数模板，返回 `T&&`。它唯一用途是在 `decltype`/`sizeof` 这类**不求值上下文**里凭空造出一个"某类型的值"，好对它做运算、调成员函数：

```cpp
// ① 查询成员调用结果的类型——不需要真的构造 Order
struct Order { int64_t qty() const; };
using QtyResult = decltype(std::declval<const Order&>().qty());
static_assert(is_same_v<QtyResult, int64_t>);   // 实测通过

// ② SFINAE 友好的返回类型：不能相加就退出重载集
template<class T, class U>
auto plus(T&& t, U&& u) -> decltype(std::declval<T>() + std::declval<U>()) {
    return std::forward<T>(t) + std::forward<U>(u);
}

// ③ declval<T>() vs declval<T&>()：值类别不同
static_assert(is_same_v<decltype(std::declval<int>()),  int&&>);
static_assert(is_same_v<decltype(std::declval<int&>()), int&>);
```

`plus` 实测：对 `std::string` 输出 `abcd`、对 `int` 输出 `3`；传入不支持 `+` 的类型时报错原文明确指向替换失败：

```
error: no matching function for call to 'plus'
note: candidate template ignored: substitution failure [with T = NoAdd &, U = NoAdd &]:
      invalid operands to binary expression ('NoAdd' and 'NoAdd')
```

这就是 `decltype` + `declval` 的威力：**编译期既能精确查询类型，又能让不可行候选安静退出重载集**。把它当普通函数调用则会被 libc++ 的内置 `static_assert` 拦下（实测原文）：

```
error: static assertion failed due to requirement '!__is_same(int, int)':
       std::declval can only be used in an unevaluated context.
```

---

## 完整可运行例子

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
using std::is_same_v;

int& getRef() { static int g = 7; return g; }
// (A) C++11 尾置返回类型
template<class It>
auto firstOfCxx11(It b, It e) -> decltype(*b) { return *b; }
// (B) C++14 decltype(auto)
template<class It>
decltype(auto) firstOfCxx14(It b, It e) { return *b; }
// (C) decltype + declval 查询成员调用结果
struct Order { int64_t qty() const; };
static_assert(is_same_v<decltype(std::declval<const Order&>().qty()), int64_t>);
// (D) SFINAE 友好的通用加法
template<class T, class U>
auto plus(T&& t, U&& u) -> decltype(std::declval<T>() + std::declval<U>()) {
    return std::forward<T>(t) + std::forward<U>(u);
}

int main() {
    int x = 42;
    const int& cx = x;
    std::cout << "decltype(x)==int        : " << is_same_v<decltype(x), int> << "\n";
    std::cout << "decltype((x))==int&     : " << is_same_v<decltype((x)), int&> << "\n";
    std::cout << "decltype(cx)==const int&: " << is_same_v<decltype(cx), const int&> << "\n";
    std::vector<int> v{10, 20, 30};
    firstOfCxx14(v.begin(), v.end()) = 99;          // 写穿到容器元素
    std::cout << "v[0] after write-through: " << v[0] << "\n";
    std::cout << "firstOfCxx11: " << firstOfCxx11(v.begin(), v.end())
              << " getRef: " << getRef() << "\n";
    std::cout << "plus(str): " << plus(std::string("ab"), std::string("cd"))
              << " plus(int): " << plus(1, 2) << "\n";
    std::vector<bool> vb{true};
    auto proxy = vb[0];                             // 代理，不是 bool
    std::cout << "vb[0] is bool? " << is_same_v<decltype(proxy), bool>
              << " sizeof=" << sizeof(proxy) << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
decltype(x)==int        : 1
decltype((x))==int&     : 1
decltype(cx)==const int&: 1
v[0] after write-through: 99
firstOfCxx11: 99 getRef: 7
plus(str): abcd plus(int): 3
vb[0] is bool? 0 sizeof=16
```

最后一行是伏笔：`-> decltype(*b)` 忠实返回 `vector<bool>` 的 16 字节代理类型，不是 `bool`——`decltype` 的"诚实"在这里变成陷阱，处理办法见 [Item 6](../ch02-auto/item06-explicitly-typed-initializer.md)。

---

## 常见错误（新手踩坑）

1. **`decltype((x))` 当 `decltype(x)` 用**。定义变量时 `decltype((x)) y = ...;` 会推成引用，绑错对象甚至改到不想改的东西。
2. **以为 `decltype` 会求值**。`decltype` 的操作数是不求值上下文，`decltype(getRef())` 不会真的调用——和 `sizeof` 一样，纯编译期查询。
3. **用 `auto` 写转发包装器**。`auto f(T&& x) { return g(std::forward<T>(x)); }` 把 `g` 返回的引用剥成值，调用方拿到拷贝。转发必须用 `decltype(auto)`。
4. **`return (x);`**。在 `decltype(auto)` 函数里 = 返回局部引用 = 悬垂。有 `-Wreturn-stack-address` 兜底，但别把安全网当设计。
5. **在求值上下文调用 `std::declval`**。它只有声明没有定义，libc++ 直接 `static_assert` 拦下（`can only be used in an unevaluated context`）。
6. **C++11 项目里写 `auto f() {...}` 期待保引用**。既过不了编译（`deduced return types are a C++14 extension`），语义上也会剥引用；C++11 只能写 `-> decltype(expr)`。另外 `decltype(cx) r;` 不初始化会报错——`decltype` 给你什么类型，你就得守什么类型的规矩。

> 和 C 的差别：C 没有 `decltype`（C11 的 `_Generic` 是选择器不是类型查询）；C 里括号不改变类型语义，所以"加括号变引用"对 C 程序员毫无直觉——记两条分支，别记特例。

---

## HFT 关联

- **泛型回调转发**：策略引擎的 `template<class F> decltype(auto) on_tick(F&& f)` 保证回调返回类型原样传递（实测 `firstOfCxx14(...) = 99;` 能写穿）。`auto` 剥掉引用会让返回订单引用的回调静默拷一份——热路径多一次拷贝，语义上调用方还可能改的是副本。
- **`decltype(auto)` + `forward` 是完美转发闭环的两个半环**：`T&&` + `std::forward` 保参数的左右值性，`decltype(auto)` 保返回值的左右值性。缺任何一个，"完美"都有洞。
- **协议字段契约钉死在编译期**：`static_assert(is_same_v<decltype(std::declval<const Tick&>().timestamp()), int64_t>)`，不构造对象就锁定字段类型；字段从 `int64_t` 改成 `uint32_t` 时所有依赖点立刻编译失败。
- **悬垂检测靠编译器**：`return (x);` 就多一对括号，评审极难看，但 `-Wreturn-stack-address` 能稳定抓到——热路径开满 `-Wall -Wextra` 不是风格洁癖，是在抓 UB。

---

## 自测题

**1. `int x = 42; const int& cx = x;` 时 `decltype(x)`、`decltype((x))`、`decltype(cx)`、`decltype((cx))` 各是什么？机制上为什么不同？**
<details><summary>答案</summary>
`decltype(x)`=`int`，`decltype((x))`=`int&`，`decltype(cx)`=`const int&`，`decltype((cx))`=`const int&`。机制：`x`/`cx` 是未加括号的变量名，走分支一返回**声明类型**；`(x)`/`(cx)` 是表达式，走分支二按值类别——左值给 `T&`。后两者结果相同纯属巧合（声明类型本就是 `const int&` vs 左值给 `const int&`），别记特例，记两条分支。实测四个 `static_assert` 全部通过。
</details>

**2. `auto` 返回类型推导和 `decltype(auto)` 在 `return getRef();` 时各得到什么？对调用方有什么实际差别？**
<details><summary>答案</summary>
`auto` 版返回 `int`（剥掉引用，得到一份拷贝）；`decltype(auto)` 版返回 `int&`。差别：拷贝版 `wrapper() = 100;` 编译失败（`error: expression is not assignable`），只读场景每次多一次拷贝；引用版能写穿到原对象（实测输出 `7` → `100`）。
</details>

**3. 为什么 C++11 里 `template<class It> auto firstOf(It b, It e) -> decltype(*b)` 必须写成尾置形式？前置写法会怎样？**
<details><summary>答案</summary>
C++11 的返回类型写在参数列表之前，那时 `b` 还没进入作用域。前置写法实测报错 `error: use of undeclared identifier 'b'`。尾置返回类型把返回类型挪到参数之后，`b` 已声明才写得出来。C++14 起可简化为 `decltype(auto) firstOf(It b, It e)`（实测在 `-std=c++11` 下报 `deduced return types are a C++14 extension`）。
</details>

**4. `decltype(auto) f() { int x = 0; return (x); }` 有什么问题？不加 `-Wall` 能抓到吗？**
<details><summary>答案</summary>
`(x)` 是左值表达式，返回类型被推成 `int&`——返回局部变量的引用，函数返回后 `x` 销毁，调用方拿到悬垂引用，UB。能抓到：clang 报 `-Wreturn-stack-address`，且**默认就开**（本机实测不加 `-Wall` 同样报）。但警告不是保证，写 `decltype(auto)` 函数要主动排雷。
</details>

**5. `std::declval` 是干什么的？`decltype(std::declval<int>())` 和 `decltype(std::declval<int&>())` 各是什么？能当普通函数调用吗？**
<details><summary>答案</summary>
`declval<T>()` 只有声明没有定义、返回 `T&&`，用途是在 `decltype`/`sizeof` 这类不求值上下文里凭空造出一个某类型的值，好对它做运算或调成员函数（如 `decltype(std::declval<const Order&>().qty())`）。实测 `decltype(std::declval<int>())` 是 `int&&`，`decltype(std::declval<int&>())` 是 `int&`。不能当普通函数调用——libc++ 内置 `static_assert` 报 `std::declval can only be used in an unevaluated context`。
</details>

**6. 为什么标准库的转发设施用 `decltype(auto)` 而不是 `auto` 做返回类型？**
<details><summary>答案</summary>
因为转发的定义是"类型和值类别原样传递"。`auto` 的推导规则（来自模板推导）主动剥掉引用和顶层 const——被转发函数返回 `int&`，`auto` 包装后变成 `int`，引用丢了、语义变了。`decltype(auto)` 用 `decltype` 规则，`return std::forward<T>(x);` 是 `T&` 就给 `T&`、是 `T&&` 就给 `T&&`，一个比特都不丢。
</details>

---

## 参考与延伸

- 上一节：[Item 2 理解 auto 类型推导](item02-auto-type-deduction.md)
- 下一节：[Item 4 查看推导结果](item04-viewing-deduced-types.md)
- 相关：[Item 6 显式类型初始化习惯](../ch02-auto/item06-explicitly-typed-initializer.md)（`decltype` 忠实推出的代理类型怎么处理）
- 回到：[第 1 章 类型推导](README.md)
