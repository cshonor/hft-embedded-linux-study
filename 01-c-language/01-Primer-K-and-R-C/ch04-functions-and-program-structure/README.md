# 第 4 章 函数与程序结构

**Functions and Program Structure**

## 本章讲什么

**模块化编程、多文件工程、作用域/链接、`static`/`extern`、递归、预处理**。承接第 3 章控制流，为第 5 章指针与内存布局铺路。无复杂内存运算，核心是**工程组织规则** —— Linux 内核、EDK2、DPDK 均遵循本章规范。

## 学习重点

- **`static` 两种作用**（局部生命周期 vs 文件内可见性）— 面试高频
- **函数原型、值传递、`extern` 单定义规则**
- **头文件 + 多 `.c` 编译链接**；逆波兰计算器综合案例（4.8）
- **递归栈开销** vs 迭代；**条件编译** 读内核/UEFI 必备

## 场景映射

| 方向 | 本章技能 |
|------|----------|
| HFT | 模块拆分、`static` 隔离、全局状态、多文件分层 |
| OS / UEFI | `static` 私有 helper、`extern` 跨文件符号、链接 |
| 嵌入式 / 飞控 | 驱动模块化、平台 `#ifdef`、全局传感器状态 |

## 小节

- [4.1 函数的基本知识](./4.1-函数的基本知识.md) — 返回类型、定义 vs 原型（模样/实现）、值传递
- [4.2 返回非整型值的函数](./4.2-返回非整型值的函数.md) — `return` 隐式转换、小转大/大转小精度、void
- [4.3 外部变量](./4.3-外部变量.md) — 函数外定义、全程有效、共享状态与全局变量风险
  - [4.3.1 程序内存布局](./4.3.1-程序内存布局.md) — 栈/堆/全局静态/常量/代码五区总览
- [4.4 作用域规则](./4.4-作用域规则.md) — 局部/文件/函数(goto)/块四种作用域；链接与遮蔽
- [4.5 头文件](./4.5-头文件.md) — `.h` 集中声明、代码复用、增量编译、模块化协作
- [4.6 静态变量](./4.6-静态变量.md) — `static` 四种用法总览：文件变量/`static const`/函数内局部/`static` 函数
- [4.7 寄存器变量](./4.7-寄存器变量.md) — `register` 局部/形参、寄存器建议、不能取址、仅函数内
  - [4.7.1 `volatile` 限定符](./4.7.1-volatile限定符.md) — 防编译器缓存、硬件/异步修改、与 `register` 对比
- [4.8 程序块结构](./4.8-程序块结构.md) — RPN 逆波兰计算器：多文件分工、`calc.h`/`stack.h`、`static`/`extern` 综合
- [4.9 初始化](./4.9-初始化.md)
- [4.10 递归](./4.10-递归.md)
- [4.11 C 预处理器](./4.11-c-preprocessor/4.11-C预处理器.md)
  - [4.11.1 文件包含](./4.11-c-preprocessor/4.11.1-文件包含.md)
  - [4.11.2 宏替换](./4.11-c-preprocessor/4.11.2-宏替换.md)
  - [4.11.3 条件包含](./4.11-c-preprocessor/4.11.3-条件包含.md)

---

## 章节自测

> 每题对应一个小节。看代码 → 想答案 → 点开验证。

### Q1: static 两种用法

```c
// file_a.c
static int count = 0;     // (1)
static int helper(void) { // (2)
    return ++count;
}

int get_count(void) {
    return helper();
}

// file_b.c — 能否直接调用 helper()？能否直接访问 count？
```

> `(1)` 和 `(2)` 的 `static` 各起什么作用？别的 `.c` 文件能访问它们吗？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` `static int count` — 文件作用域，**仅限 file_a.c 内可见**，其他 `.c` 文件无法 `extern` 引用
- `(2)` `static int helper(void)` — 函数仅限本文件可见，其他文件无法调用

**解析：** `static` 在函数外 → 限制文件内可见性（内部链接）。`static` 在函数内 → 变量生命周期延长到整个程序运行期（值在调用间保持）。两者完全不同的语义。

其他文件不能调用 `helper()`，也不能 `extern` 引用 `count`。

**复习：** → [4.6 静态变量](./4.6-静态变量.md)

</details>

### Q2: extern 跨文件

```c
// config.c
int max_connections = 100;

// main.c
extern int max_connections;

int main() {
    printf("%d\n", max_connections);
    return 0;
}
```

> `extern` 做什么？如果去掉 `extern` 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：** `extern int max_connections;` 是**声明**（不是定义），告诉编译器「这个变量在别处定义，链接时找」。去掉 `extern` 变成定义，会导致链接时**重复定义**错误。

**解析：** 声明不分配内存，定义分配内存。一个变量只能定义一次，但可以声明多次。实际工程中把 `extern` 声明放在 `.h` 头文件中统一管理。

**复习：** → [4.3 外部变量](./4.3-外部变量.md) · [4.5 头文件](./4.5-头文件.md)

</details>

### Q3: 函数原型

```c
// 情况 A：有原型
double sqrt_approx(double x);  // 原型
int main() {
    printf("%f\n", sqrt_approx(2));  // OK
    return 0;
}

// 情况 B：无原型
int main() {
    printf("%f\n", sqrt_approx(2));  // 会怎样？
    return 0;
}
double sqrt_approx(double x) { return x / 1.414; }
```

> 情况 B 会发生什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 情况 B 在 C89 中允许隐式声明（编译器假设返回 `int`），但会**产生错误结果**——`sqrt_approx` 返回 `double`，但 `main` 以为返回 `int`，寄存器/截断可能出错。C99 起**隐式声明已不合法**，必须报错。

**教训：** 总是写函数原型。把原型放头文件里，`#include` 它。

**复习：** → [4.1 函数的基本知识](./4.1-函数的基本知识.md) — 原型 vs 定义

</details>

### Q4: 递归与栈

```c
int factorial(int n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

// 如果调用 factorial(100000) 会怎样？
```

> `factorial(5)` 返回多少？调用 `factorial(100000)` 会发生什么？

<details>
<summary>答案与复习指引</summary>

**答案：** `factorial(5) = 120`。`factorial(100000)` → **栈溢出（stack overflow）**。

**解析：** 每层递归在栈上压一个栈帧。`n=100000` 意味着 10 万层调用，默认栈大小（Linux 通常 8MB）会耗尽。另外 `int` 会溢出（`13!` 就超过 `int` 范围）。

**教训：** 递归深度可控时用递归（树遍历等），深度不可控时改迭代。

**复习：** → [4.10 递归](./4.10-递归.md)

</details>

### Q5: 宏替换

```c
#define SQUARE(x)  x * x
#define DBL(x)    ((x) * 2)

int a = 3;
int r1 = SQUARE(a + 1);   // (1)
int r2 = DBL(a + 1);      // (2)
```

> `(1)` 和 `(2)` 各是多少？为什么结果不同？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` `SQUARE(a + 1)` → `a + 1 * a + 1` → `3 + 3 + 1 = 7`（不是 16！）
- `(2)` `DBL(a + 1)` → `((a + 1) * 2)` → `((3 + 1) * 2) = 8`

**解析：** 宏是**文本替换**，不做表达式求值。`SQUARE(x)` 展开为 `x * x`，`x = a + 1` 展开为 `a + 1 * a + 1`（先乘后加）。正确写法：`#define SQUARE(x) ((x)*(x))`，参数加括号。

**教训：** 宏参数必须加括号，能避免大部分优先级陷阱。

**复习：** → [4.11.2 宏替换](./4.11-c-preprocessor/4.11.2-宏替换.md)

</details>

### Q6: 条件编译

```c
#ifdef DEBUG
    printf("debug: x=%d\n", x);
#endif

#if VERSION >= 2
    // V2 code
#elif VERSION == 1
    // V1 code
#else
    // legacy
#endif
```

> 条件编译用什么场景？内核和驱动里为什么离不开它？

<details>
<summary>答案与复习指引</summary>

**答案：** 条件编译用于：
1. **调试开关**：`#ifdef DEBUG` 控制调试输出，发布版编译掉
2. **平台适配**：`#ifdef __x86_64__` / `#ifdef __aarch64__` 区分架构
3. **内核配置**：`#ifdef CONFIG_SMP` 对称多处理器、`#ifdef CONFIG_PREEMPT` 抢占
4. **头文件防重复**：`#ifndef HEADER_H ... #endif`（include guard）

内核 `.config` 生成 `#define CONFIG_XXX`，编译时选择性地编译代码。没有条件编译，内核源码无法做到一份代码适配所有架构和配置。

**复习：** → [4.11.3 条件包含](./4.11-c-preprocessor/4.11.3-条件包含.md)

</details>

---

## 代码自测

**题目 1：** 以下代码体现了 K&R 的什么编程风格？extern 的作用是什么？
```c
// file1.c
int shared_var = 42;
// file2.c
extern int shared_var;
int main() { return shared_var; }
```

<details>
<summary>参考答案</summary>

extern 声明 shared_var 在别处定义——它告诉编译器变量的类型和链接属性但不分配存储。file2.c 通过 extern 引用 file1.c 中的全局变量。K&R 风格中，共享变量通过 extern 在头文件中声明，在 .c 文件中定义。现代编程建议尽量减少全局变量，用函数参数传递数据。

</details>
