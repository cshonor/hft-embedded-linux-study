# 第一章 让自己习惯 C++

共 4 条条款。

## 条款

- [条款 1：视 C++ 为一个语言联邦](./item01-视C++为一个语言联邦.md)
- [条款 2：尽量以 const、enum、inline 替换 #define](item02-尽量以const、enum、inline替换#define.md)
- [条款 3：尽可能使用 const](./item03-尽可能使用const.md)
- [条款 4：确定对象被使用前已先被初始化](./item04-确定对象被使用前已先被初始化.md)


## 章节摘要

让自己习惯 C++：视 C++ 为语言联邦（C/面向对象/模板/STL）、用 `const`/`enum`/`inline` 替代 `#define`、尽可能使用 `const`、对象使用前确保初始化。

## 代码自测

### Q1: #define vs const

```cpp
#define MAX_SIZE 256
// vs
const int MAX_SIZE = 256;
```

> 两种方式有什么区别？`#define` 有什么问题？

<details>
<summary>答案与复习指引</summary>

**`#define` 问题：**
1. 无类型检查——预处理器纯文本替换，不进符号表，调试看不到名字
2. 作用域不受限——`#define` 从定义处到文件尾全局有效，不遵循 C++ 作用域规则
3. 多次求值——`#define SQUARE(x) ((x)*(x))` 调用 `SQUARE(i++)` 会递增两次

**`const` 优势：** 有类型、可调试、遵循作用域、可用于 `constexpr`/模板参数。

**`enum` hack：** `enum { MAX_SIZE = 256 };` 在旧编译器中避免 `static const` 的地址取用问题（不分配内存）。

**复习：** → [条款 2：尽量以 const、enum、inline 替换 #define](item02-尽量以const、enum、inline替换#define.md)
</details>

### Q2: const 返回值

```cpp
class Rational { /* ... */ };
const Rational operator*(const Rational& lhs, const Rational& rhs);
Rational a, b, c;
(a * b) = c;   // 如果返回 const：合法吗？
(a * b) == c;  // 如果返回 const：合法吗？
```

> 返回 `const Rational` 时，两个操作分别合法吗？为什么返回 const？

<details>
<summary>答案与复习指引</summary>

**`(a*b) = c` 编译错误**（const 返回值不可赋值）——防止"对临时对象赋值"的无意义操作。
**`(a*b) == c` 合法**——const 不影响比较。

**返回 const 的目的：** 防止 `if (a * b = c)` 这种把 `==` 误写为 `=` 的 bug（对临时结果赋值被阻止）。

**注意：** 现代 C++（C++11+）倾向不返回 const——移动语义需要非 const 返回值才能触发移动。但 `operator*` 等返回新对象的场景仍可考虑。

**复习：** → [条款 3：尽可能使用 const](./item03-尽可能使用const.md)
</details>

### Q3: 初始化顺序

```cpp
// file1.cpp
extern int y;
int x = y + 1;   // x 依赖 y
// file2.cpp
extern int x;
int y = x + 1;   // y 依赖 x
```

> `x` 和 `y` 的最终值是什么？如何避免这个问题？

<details>
<summary>答案与复习指引</summary>

**未定义——"静态初始化顺序灾难"（static init order fiasco）。** 跨翻译单元的全局对象构造顺序未指定。如果 `x` 先构造，`y` 还未初始化（值为 0），`x = 0 + 1 = 1`，然后 `y = 1 + 1 = 2`。如果 `y` 先构造则反过来。

**修复：用 Meyers Singleton（函数内 static）：**
```cpp
int& get_y() { static int y = /* init */; return y; }
int x = get_y() + 1;  // 保证 get_y() 先执行
```
函数内 `static` 在首次调用时初始化，保证顺序。

**复习：** → [条款 4：确定对象被使用前已先被初始化](./item04-确定对象被使用前已先被初始化.md)
</details>
