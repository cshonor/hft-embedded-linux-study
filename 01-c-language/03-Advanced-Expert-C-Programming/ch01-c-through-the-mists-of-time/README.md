# 第 1 章 C：穿越时空的迷雾

**C Through the Mists of Time** — Peter van der Linden, *Expert C Programming*

## 本章目标

建立 C 语言的 **历史与标准观**：从 Multics/B/Unix 理解 **编译器优先** 设计；掌握 K&R → ANSI C 的关键差异（原型、`void`、`const`）；区分 **hosted/freestanding**、**implementation-defined/undefined**；知道 **translation limits** 与 **ISO 标准文档结构**；能查阅标准、识别 **quiet change**。为全书「为什么 C 长这样」打底，而非重复 K&R 语法教程。

## 核心思想

| 主题 | 要点 |
|------|------|
| **编译器优先** | 语法极简、类型映射硬件、单遍编译时代遗留（数组衰变、隐式 int） |
| **极简内核 + 库** | I/O 在 stdio；条件编译在 cpp；语言小、库大 |
| **历史溯源** | 1972 C、1973 Unix 内核重写 → C 成为系统语言 |
| **标准即契约** | 符合性分层；UB 不是「编译器 bug」 |
| **扩展与 pragma** | far/near、GNU attribute、不可移植但系统代码常用 |

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **无硬性前置** | 本章偏背景与标准，可与 K&R 并行 |
| **[01-Primer-K-and-R-C](../../01-Primer-K-and-R-C/)** | 已会基本 C 语法时收获最大 |
| **Embedded C ch04**（可选） | 编译链接流程有助于理解 translation unit |

## 环境

- **编译器**：GCC 或 Clang，`gcc --version`
- **标准草案**（可选）：[WG14 open-std](https://www.open-std.org/jtc1/sc22/wg14/) N1570（C11）等
- **参考**：cppreference.com、GCC Manual「C Dialect Options」
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/03-Advanced-Expert-C-Programming/ch01-c-through-the-mists-of-time/demo

make all
./demo01_kr_vs_ansi
./demo02_macro_trap

# ANSI 原型：取消 demo01 中错误实参注释 → 编译失败
# gcc -std=c89 -Wall demo/demo01_kr_vs_ansi.c

# 查看标准版本宏
echo | gcc -E -dM -std=c11 - | grep __STDC_VERSION__

make clean
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 史前与早期** | **1.1–1.2** | Multics→B→C；Unix 重写；下标 0、衰变、float 提升、`register` |
| **2 库与预处理** | **1.3** | stdio 设计；`#include`/`#define`/`#ifdef`；宏陷阱 |
| **3 K&R 与 ANSI** | **1.4–1.5** | 无原型、implicit int；C89 原型、`void`、`const`、标准库 |
| **4 符合性** | **1.6–1.7** | hosted/freestanding；impl-defined vs UB；translation limits |
| **5 读标准** | **1.8–1.9** | 标准层次；Constraints/Semantics；Annex J/C |
| **6 演进与 pragma** | **1.10–1.11** | quiet change；`#pragma` 与编译器扩展 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | ANSI 原型 vs K&R 旧式；错误类型实参 | **1.4**、**1.5** |
| **demo02** | 宏括号陷阱 `SQUARE` vs `BAD_SQUARE` | **1.3** |

## 高频考点 / 面试题

1. **C 为什么数组下标从 0 开始？** → 指针+偏移模型、地址算术、PDP-11 代码生成（**1.2**）
2. **K&R 与 ANSI 函数声明有何区别？举例说明无原型的危害。** → `int f()` vs `int f(int a)`；实参类型不检查（**1.4**，**demo01**）
3. **implementation-defined、unspecified、undefined 有何区别？各举一例。** → `sizeof(int)` / 求值顺序 / 有符号溢出、越界（**1.6**）
4. **什么是 hosted 与 freestanding 实现？Linux 内核属于哪类？** → 标准库完整 vs 最小集；内核近似 freestanding + GNU C（**1.6**、**1.8**）
5. **宏 `#define SQUARE(x) x*x` 有何问题？如何修正？** → 优先级、副作用、用括号或 inline（**1.3**，**demo02**）

**拓展（书中常问）：**

- 编译器优先设计如何影响 C 语法？
- 何谓「安静的改变」？strict aliasing 一例（**1.10**）
- `far`/`near` 指针出现在什么历史背景？（**1.6**）
- C 标准中 translation limits 解决什么问题？（**1.7**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **K&R** 基础语法；无 Expert C 硬性前置 |
| 后置 | **ch02** 语言特性与 UB；**ch03** 声明读法；**ch04–ch10** 数组/指针/链接/内存 |
| 关联 | **Embedded C** 编译链接、GNU C 扩展；**内核/DPDK** freestanding 与性能 pragma |

## 小节

- [1.1 C语言的史前阶段](./1.1-C语言的史前阶段.md)
- [1.2 C语言的早期体验](./1.2-C语言的早期体验.md)
- [1.3 标准I/O库和C预处理器](./1.3-标准IO库和C预处理器.md)
- [1.4 K&R C](./1.4-KR-C.md)
- [1.5 今日之ANSI C](./1.5-今日之ANSI-C.md)
- [1.6 它很棒，但它符合标准吗](./1.6-它很棒-但它符合标准吗.md)
- [1.7 编译限制](./1.7-编译限制.md)
- [1.8 ANSI C标准的结构](./1.8-ANSI-C标准的结构.md)
- [1.9 阅读ANSI C标准，寻找乐趣和裨益](./1.9-阅读ANSI-C标准-寻找乐趣和裨益.md)
- [1.10 「安静的改变」究竟有多少安静](./1.10-安静的改变-究竟有多少安静.md)
- [1.11 轻松一下——由编译器定义的Pragmas效果](./1.11-轻松一下由编译器定义的Pragmas效果.md)

## 章节自测

> C 历史与标准：K&R vs ANSI、UB 分类、hosted/freestanding。看代码 → 想答案 → 点开验证。

### Q1: K&R 旧式声明

```c
/* K&R 风格 */
int add(a, b)
    int a, b;
{
    return a + b;
}

/* ANSI 风格 */
int sub(int a, int b) {
    return a - b;
}

int main() {
    printf("%d\n", add(3, 4));      // A
    printf("%d\n", add(3.0, 4.0));  // B：传 double
    printf("%d\n", sub(3.0, 4.0));  // C：传 double
    return 0;
}
```

> B 和 C 哪个更危险？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** **B 更危险**。K&R 旧式声明**不做参数类型检查**——`add(3.0, 4.0)` 传 `double`，但函数期望 `int`，编译器不警告。`double` 8 字节被当 `int` 4 字节读取 → 结果错误或 UB。

ANSI 原型 `int sub(int, int)` 会让编译器**自动转换** `3.0` → `3`（截断），安全。

**规则：** 始终用 ANSI 原型；`-Werror=implicit-function-declaration` 禁止旧式声明。

**复习：** → [1.4 K&R C](./1.4-KR-C.md) · [1.5 今日之 ANSI C](./1.5-今日之ANSI-C.md)

</details>

### Q2: implementation-defined vs undefined

```c
int x = -1;
int shifted = x >> 1;       // A

int arr[5];
printf("%d\n", arr[3]);     // B（未初始化）

char c = 200;               // C（char 符号性实现定义）
printf("%d\n", c);
```

> A、B、C 分别是 implementation-defined、unspecified 还是 undefined？

<details>
<summary>答案与复习指引</summary>

**答案：**
- A：**implementation-defined**（有符号右移——编译器文档说明是算术还是逻辑右移）
- B：**unspecified/indeterminate value**（未初始化自动变量——值不确定，读它是 UB 如果是 trap representation）
- C：**implementation-defined**（`char` 是否有符号由实现定义——`200` 可能是 200 或 -56）

**区别：** implementation-defined = 编译器必须文档化选择；undefined = 标准不约束任何行为；unspecified = 标准提供多个选项但不要求文档化。

**复习：** → [1.6 它很棒，但它符合标准吗](./1.6-它很棒-但它符合标准吗.md)

</details>

### Q3: hosted vs freestanding

```c
// hosted 环境（Linux 用户态）
#include <stdio.h>
int main(void) {
    printf("hello\n");
    return 0;
}

// freestanding 环境（内核/裸机）
void kernel_main(void) {
    // 没有 main()，没有 stdio.h
    // 用自定义 UART 输出
    uart_puts("hello\n");
}
```

> 两种环境的主要区别是什么？Linux 内核属于哪类？

<details>
<summary>答案与复习指引</summary>

**答案：**
- **hosted**：有完整标准库（`stdio.h`、`stdlib.h` 等），入口是 `main()`，OS 提供运行时
- **freestanding**：最小库集（`<stddef.h>`、`<stdint.h>` 等），入口自定义，无 OS 依赖

Linux 内核近似 freestanding + GNU C 扩展——不使用 `stdio.h`/`stdlib.h`，入口是 `start_kernel()`，自己管理内存和中断。

**HFT 关联：** DPDK 用户态驱动绕过内核，但仍在 hosted 环境；裸机 HFT 可能用 freestanding。

**复习：** → [1.6 它很棒，但它符合标准吗](./1.6-它很棒-但它符合标准吗.md)

</details>

### Q4: 数组下标为什么从 0 开始

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;

// C 语言中 arr[i] 等价于什么？
printf("%d\n", arr[2]);
printf("%d\n", *(p + 2));
printf("%d\n", 2[arr]);     // 这个合法吗？
```

> 三行 printf 分别输出什么？为什么 C 数组从 0 开始？

<details>
<summary>答案与复习指引</summary>

**答案：** 三行都输出 `30`。`arr[i]` 等价于 `*(arr + i)`，也等价于 `*(i + arr)` 即 `i[arr]`。

**从 0 开始的原因：** C 的数组下标本质是**偏移量**——`arr[0]` = `*(arr + 0)` = 首元素。PDP-11 时代地址算术直接映射硬件指令，偏移 0 = 基地址本身，最自然。

**规则：** `i[arr]` 合法但勿在生产代码使用——可读性差。

**复习：** → [1.2 C 语言的早期体验](./1.2-C语言的早期体验.md)

</details>
