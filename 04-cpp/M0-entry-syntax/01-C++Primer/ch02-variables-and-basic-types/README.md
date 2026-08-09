# 第 2 章 变量和基本类型

数据类型是程序的基础。本章讲述 C++ 的基本内置类型、复合类型、常量限定符、类型推断机制，并初步介绍如何自定义数据结构。

## 小节

- [2.1 基本内置类型](./2.1-基本内置类型.md)
- [2.2 变量](./2.2-variables/2.2-变量.md)
  - [2.2.1 标识符与未初始化](./2.2-variables/2.2.1-标识符与未初始化.md)
  - [2.2.2 变量、常量与 static](./2.2-variables/2.2.2-变量常量与static.md)
  - [2.2.3 变量遮蔽](./2.2-variables/2.2.3-变量遮蔽.md)
  - [2.2.4 extern 与头文件分工](./2.2-variables/2.2.4-extern与头文件分工.md)
- [2.3 复合类型](./2.3-compound-types/2.3-复合类型.md)
  - [2.3.1 引用](./2.3-compound-types/2.3.1-引用.md)
  - [2.3.2 指针与 nullptr](./2.3-compound-types/2.3.2-指针与nullptr.md)
- [2.4 const 限定符](./2.4-const-qualifier/2.4-const限定符.md)
  - [2.4.1 const 基础与 constexpr](./2.4-const-qualifier/2.4.1-const基础与constexpr.md)
  - [2.4.2 const 口诀与声明语法](./2.4-const-qualifier/2.4.2-const口诀与声明语法.md)
  - [2.4.3 constexpr 详解](./2.4-const-qualifier/2.4.3-constexpr详解.md)
- [2.5 处理类型](./2.5-处理类型.md)
- [2.6 自定义数据结构](./2.6-custom-data-structures/2.6-自定义数据结构.md)
  - [2.6.1 Sales_data 入门与聚合](./2.6-custom-data-structures/2.6.1-Sales_data入门与聚合.md)
  - [2.6.2 类内初始值](./2.6-custom-data-structures/2.6.2-类内初始值.md)
  - [2.6.3 Sales_data 完整版](./2.6-custom-data-structures/2.6.3-Sales_data完整版.md)
  - [2.6.4 友元与前置声明](./2.6-custom-data-structures/2.6.4-友元与前置声明.md)
  - [2.6.5 头文件分离与保护](./2.6-custom-data-structures/2.6.5-头文件分离与保护.md)
- [小结](./2.7-小结.md)


## 章节摘要

C++ 的基本内置类型（`int`/`double`/`char`/`bool`）、变量初始化规则、复合类型（引用 `&` 和指针 `*`）、`const` 限定符与 `constexpr`、类型别名（`using`/`typedef`）以及自定义数据结构。

### 和 C 的区别

| C | C++ |
|---|-----|
| 无引用类型，全靠指针 | 有引用 `&`（别名，不可空） |
| `const` 默认外部链接 | `const` 默认内部链接（类似 C 的 `static`） |
| `#define PI 3.14` | `constexpr double PI = 3.14;`（类型安全） |
| `typedef` | `using`（支持模板化） |
| `NULL`（整数 0） | `nullptr`（专用空指针类型） |
| 初始化列表 `{1,2,3}` 受限 | 大括号初始化统一可用 |

## 章节自测

### Q1: 引用绑定

```cpp
int ival = 1024;
int &refVal = ival;
refVal += 10;
// int &ref2;  // 注释掉的一行
```

> `ival` 现在是多少？注释掉的那行为什么编译错误？引用和指针的三个本质区别是什么？

<details>
<summary>答案与复习指引</summary>

**ival = 1034。** `refVal` 是 `ival` 的别名，`refVal += 10` 等同于 `ival += 10`。

**编译错误原因：** 引用必须在声明时绑定，不能只声明不初始化。

**引用 vs 指针：**
1. 引用必须初始化，指针可以不初始化
2. 引用绑定后不可改变（不能重新绑定到另一个对象），指针可以重新指向
3. 引用不能为空（不指向"无对象"），指针可以是 `nullptr`

**复习：** → [2.3.1 引用](./2.3-compound-types/2.3.1-引用.md)
</details>

### Q2: const 与链接性

```cpp
// file1.cpp
const int buf_size = 512;
// file2.cpp
const int buf_size = 1024;  // 会冲突吗？
```

> 两个文件分别定义 `const int buf_size`，会冲突吗？如果改成 `int`（非 const）呢？

<details>
<summary>答案与复习指引</summary>

**不冲突。** C++ 中 `const` 变量默认是**内部链接**的（相当于加了 `static`），每个文件有自己的副本。

**如果改成非 const `int`：** 会冲突——非 const 全局变量是外部链接，两个文件定义同名变量会链接错误。

**要共享 const 变量：** 在头文件声明 `extern const int buf_size;`，在一个 .cpp 中定义 `extern const int buf_size = 512;`。

**和 C 的区别：** C 中 `const` 默认是外部链接（和普通变量一样），要加 `static` 才是内部链接。

**复习：** → [2.4.1 const 基础与 constexpr](./2.4-const-qualifier/2.4.1-const基础与constexpr.md)
</details>

### Q3: constexpr 编译期常量

```cpp
constexpr int mf = 20;
constexpr int limit = mf + 1;
// constexpr int sz = get_size();  // 仅当 get_size 是 constexpr 函数时合法
```

> `constexpr` 和 `const` 有什么区别？为什么 `sz` 那行可能编译失败？

<details>
<summary>答案与复习指引</summary>

**区别：**
- `const`：表示"运行时不可修改"，但值不一定在编译期已知（如 `const int x = rand();`）
- `constexpr`：表示"编译期可求值"，值必须在编译期确定

**`sz` 行：** 只有当 `get_size()` 是 `constexpr` 函数（编译期可求值）时才合法。普通函数的返回值不是编译期常量。

**复习：** → [2.4.3 constexpr 详解](./2.4-const-qualifier/2.4.3-constexpr详解.md)
</details>

### Q4: 指针与 const

```cpp
const double pi = 3.14;
// double *ptr = &pi;       // A
const double *cptr = &pi;   // B
// *cptr = 42;              // C
```

> A、C 行分别有什么问题？"指向 const 的指针"和"const 指针"的区别是什么？

<details>
<summary>答案与复习指引</summary>

**A 行错误：** 不能用 `double*` 指向 `const double`——否则可以通过指针修改只读变量。
**C 行错误：** `cptr` 指向 `const double`，不能通过它修改值。

**两种 const 指针：**
- `const double *p`：指向 const 的指针——不能通过 `p` 改值，但 `p` 可以指向别的对象（"底 const"）
- `double *const p = &x`：const 指针——`p` 本身不可改（不能指向别的），但可以通过 `p` 改 `x` 的值（"顶 const"）

**口诀：** `const` 在 `*` 左边修饰指向的值，在 `*` 右边修饰指针本身。

**复习：** → [2.4.2 const 口诀与声明语法](./2.4-const-qualifier/2.4.2-const口诀与声明语法.md)
</details>

### Q5: auto 类型推导

```cpp
int i = 0, &r = i;
auto a = r;       // a 的类型是？
const int ci = i;
auto b = ci;      // b 的类型是？
auto &c = ci;     // c 的类型是？
```

> a、b、c 分别是什么类型？

<details>
<summary>答案与复习指引</summary>

- `a` 是 `int`：`r` 是 `i` 的引用，`auto` 推导时忽略引用性，得到 `int`
- `b` 是 `int`：`ci` 是 `const int`，但 `auto` 按值推导会忽略顶层 const，得到 `int`
- `c` 是 `const int&`：引用推导保留底层 const，`c` 是指向 `const int` 的引用

**关键规则：** `auto` 按值推导时忽略引用和顶层 const；按引用推导时保留底层 const。

**复习：** → [2.5 处理类型](./2.5-处理类型.md)
</details>
