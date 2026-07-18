# 第 10 章 再论指针

**More About Pointers** — Peter van der Linden, *Expert C Programming*

## 本章目标

在 **[ch09 再论数组](../ch09-more-about-arrays/)** 的 **decay / 多维布局** 基础上 **收束指针专题**：区分 **连续 `int[M][N]`** 与 **分散 `int**`** 的内存拓扑；掌握 **Iliffe 向量**（**`argv`**、**`char *str[]`**）；熟练 **一维/二维传参**（**`int a[]` vs `int a[][N]` vs `int**`**）；牢记 **`const` 与指针四组合** 及 **`typedef Str` 陷阱**；理解 **返回数组指针** 的生命周期；会用 **`malloc`/`void*`/函数指针/多级指针** 构建动态结构与回调表。衔接 **[ch03 声明](../ch03-analyzing-c-declarations/)**、**[ch04 数组≠指针](../ch04-arrays-are-not-pointers/)**、**[ch05 链接](../ch05-thinking-of-linking/)**、**[ch06 运行时](../ch06-runtime-data-structures/)**、**[ch07 内存](../ch07-adventures-in-memory/)**、**[ch11 C++](../ch11-cpp-for-c-programmers/)**。

## 连续 vs 分散：本章核心对照

| 维度 | **`int[M][N]` 连续** | **`int **` 锯齿 / Iliffe** |
|------|----------------------|----------------------------|
| **物理布局** | 一块 M×N int | 指针表 + 各行（或 rodata 串） |
| **decay / 传参** | **`int (*)[N]`** | **`int **`** |
| **内维 N** | 形参 **必须** 给出 | 另传 **`cols`** |
| **典型** | 栈/静态矩阵 | **`malloc` 矩阵、**`argv`** |
| **错误** | 用 **`int**`** 接收 | 用 **`int[][N]`** 接收 |

## `const` 与指针速查

| 声明 | 改指向 | 改 `*p` | 口诀 |
|------|--------|---------|------|
| **`const int *p`** | ✅ | ❌ | 指向常量 |
| **`int *const p`** | ❌ | ✅ | 指针常量 |
| **`const int *const p`** | ❌ | ❌ | 都常量 |
| **`typedef char* Str; const Str s`** | ❌ | ✅ | **陷阱**：≠ `const char*` |

见 **10.4**、**demo01_const_ptr**。

## `char*` vs `char[]`（rodata）

| | **`char *p = "..."`** | **`char buf[] = "..."`** |
|--|------------------------|---------------------------|
| **数据位置** | rodata（只读） | 栈上副本（可写） |
| **改 `p[0]`** | UB / SIGSEGV | OK |
| **Iliffe** | **`char *lines[]`** 各行指字面量 | **`char lines[][N]`** 内嵌 |

见 **10.2**、**10.4**、**ch09 9.5**。

## 函数指针与工程模板

**[function-pointer-typedef-templates.md](./function-pointer-typedef-templates.md)** — 回调、IRQ handler、命令表、ISR 向量、**`signal`** 原型等 **typedef 模板**。

```c
typedef int (*MathFunc)(int, int);
typedef void (*Callback)(void);
typedef void (*SigHandler)(int);   /* signal 简化 */
```

见 **10.7**、**demo02_func_ptr**、**[ch03 3.8](../ch03-analyzing-c-declarations/3.8-理解所有分析过程的代码段.md)**。

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch04](../ch04-arrays-are-not-pointers/)** | **数组≠指针**、**`extern`** 陷阱（**4.1–4.5**） |
| **[Expert C ch09](../ch09-more-about-arrays/)** | **decay**、**二维 `arr` vs `arr[0]`**、**`argv`**（**9.1–9.6**） |
| **[Expert C ch03](../ch03-analyzing-c-declarations/)** | **`*[N]` vs `(*)[N]`**、**typedef**（**3.3, 3.5–3.8**） |
| **[Expert C ch07](../ch07-adventures-in-memory/)** | 栈/堆/静态、**`malloc`**、对齐 |
| **[Expert C ch05](../ch05-thinking-of-linking/)** | 链接与符号 |
| **[Expert C ch06](../ch06-runtime-data-structures/)** | 运行时布局、**`argv`** 构造 |

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 布局** | **10.1** | 连续 vs 分散；**指针类型 → 步长** |
| **2 Iliffe** | **10.2** | **`char **argv`**；**`char *str[]`** |
| **3 锯齿** | **10.3** | 逐行 **`malloc`**；释放顺序 |
| **4 一维传参** | **10.4** | decay；**`const` 四组合**；**`typedef` 陷阱** |
| **5 二维传参** | **10.5** | **`int a[][N]`** / **`int (*a)[N]`**；**≠ `int**`** |
| **6 返回数组** | **10.6** | 栈悬垂；static / malloc / 出参缓冲 |
| **7 动态与回调** | **10.7** | **`void*`**；**`char**`**；**函数指针** |
| **8 收束** | **10.8** | 野指针；**NULL**；检验局限 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_const_ptr** | **`const int *`** vs **`int *const`** | **10.4** |
| **demo02_func_ptr** | **`typedef MathFunc`**；**`ops[]`** 分发 | **10.7** |
| **demo03_double_ptr** | **`alloc_str(char**)`** 出参 | **10.3, 10.7** |
| **demo04_void_ptr** | **`void*`** 解引用 cast；**`malloc`** | **10.7** |
| **demo05_jagged_vs_2d** | 连续 **`int[2][3]`** vs **`int**`** 地址对比 | **10.1, 10.3, 10.5** |

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch10-more-about-pointers/demo
cd demo01_const_ptr && make && ./demo01 && cd ..
cd demo02_func_ptr && make && ./demo02 && cd ..
cd demo03_double_ptr && make && ./demo03 && cd ..
cd demo04_void_ptr && make && ./demo04 && cd ..
cd demo05_jagged_vs_2d && make && ./demo05 && cd ..
```

## 高频考点 / 面试题

1. **`int m[2][4]` 与 `int **p` 内存布局与传参有何不同？能否 `void f(int **a)` 接收 `m`？** → 前者连续 8 个 int、退化 **`int (*)[4]`**；后者指针表+分散行；**不可混传**（**10.1, 10.5**, **demo05**）

2. **写出 `const int *p`、`int *const p`、`const int *const p` 三者差异；`typedef char* Str; const Str s` 是什么？** → 见 **`const` 速查表**；**`s` 是 `char* const`**，`*s` 仍可改（**10.4**, **demo01**, **ch03 3.5**）

3. **`char *s = "hello"` 与 `char s[] = "hello"` 能否 `s[0]='H'`？** → 指针版 **不可**（rodata）；数组版 **可以**（栈副本）（**10.4**, **10.2**, **ch09 9.5**）

4. **函数返回局部数组为何 UB？合法替代？** → 栈销毁后悬垂；**static / malloc / 调用方缓冲**（**10.6**, **ch07**）

5. **`void*` 能否 `*vp` 或 `vp++`？`malloc` 为何能赋给 `int*`？** → **不能**直接解引用/算术；**`malloc` 返回 `void*`** 隐式转任意对象指针（**10.7**, **demo04**）

**拓展：**

- **读 `void (*signal(int, void (*)(int)))(int)`**（**10.7**, **function-pointer-typedef-templates.md §5**）
- **`p2-p1` 何时合法？** — 同一数组对象内（**10.7**）
- **锯齿矩阵如何正确释放？** — 先各行 **`free(m[r])`** 再 **`free(m)`**（**10.3**）
- **C 如何用函数表模拟多态？** → **ch11 11.15**

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[ch04](../ch04-arrays-are-not-pointers/)** 数组≠指针；**[ch09](../ch09-more-about-arrays/)** decay 与多维；**[ch03](../ch03-analyzing-c-declarations/)** 声明 |
| 关联 | **[ch05](../ch05-thinking-of-linking/)** 链接；**[ch06](../ch06-runtime-data-structures/)** 运行时；**[ch07](../ch07-adventures-in-memory/)** 堆栈；**[ch08](../ch08-halloween-vs-christmas/)** 软件规则之难 |
| 后置 | **[ch11](../ch11-cpp-for-c-programmers/)** C++ 引用、RAII、虚函数 |
| 全书 | **ch04 立论 → ch09 数组深化 → ch10 指针收束 → ch11 C++ 升华** |

## 辅助材料

- **[function-pointer-typedef-templates.md](./function-pointer-typedef-templates.md)** — 函数指针 **typedef** 工程模板（回调 / 命令表 / ISR / signal）

## 小节

- [10.1 多维数组的内存布局](./10.1-多维数组的内存布局.md)
- [10.2 指针数组就是 Iliffe 向量](./10.2-指针数组就是Iliffe向量.md)
- [10.3 在锯齿状数组上使用指针](./10.3-在锯齿状数组上使用指针.md)
- [10.4 向函数传递一个一维数组](./10.4-向函数传递一个一维数组.md)
- [10.5 使用指针向函数传递一个多维数组](./10.5-使用指针向函数传递一个多维数组.md)
- [10.6 使用指针从函数返回一个数组](./10.6-使用指针从函数返回一个数组.md)
- [10.7 使用指针创建和使用动态数组](./10.7-使用指针创建和使用动态数组.md)
- [10.8 轻松一下——程序检验的限制](./10.8-轻松一下程序检验的限制.md)
