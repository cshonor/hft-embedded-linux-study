# 第 3 章 分析 C 语言的声明

**Analyzing C Declarations** — Peter van der Linden, *Expert C Programming*

## 本章目标

掌握 C **声明的统一读法**：把任意声明拆成 **存储类 + 基本类型 + 声明符**；熟练运用 **螺旋规则（Spiral Rule）** 与 **图表法** 解析 `*`、`[]`、`()` 嵌套；理解 **typedef 与 #define 的本质区别** 及 **PSTR 陷阱**；能口述 **书中全部例题**（含 `signal`）；为 **ch04 数组非指针**、**ch08 类型转换**、**Embedded ch07 指针** 提供类型阅读能力。

## 核心算法：螺旋规则四步

| 步骤 | 动作 |
|------|------|
| **1** | 找到 **identifier**，写「X 是 ……」 |
| **2** | **顺时针** 扫描：括号 **由内向外** 优先 |
| **3** | 遇 `[N]` → 数组（N 个元素）；遇 `(params)` → 函数；遇 `*` → 指针 |
| **4** | 函数写 **返回类型**；数组写 **元素类型**；指针写 **指向类型**；重复至基本类型 |

**优先级**：`( )` 改变顺序 → **`[]` 与 `()` 优先于 `*`**（详见 **3.3**）。

## 四组对比表（必背）

| 声明 | 主体类型 | 完整含义 |
|------|----------|----------|
| `char *str[10]` | **数组** | 10 个元素，每个元素是 `char*`（指针数组） |
| `char (*p)[10]` | **指针** | 指向含 10 个 `char` 的数组（数组指针） |
| `int (*func)(int, char)` | **指针** | 指向「接受 (int,char)，返回 int」的函数 |
| `int *f(int)` | **函数** | 接受 int，**返回 int\***（对比：`int (*fp)(int)` 才是函数指针） |

**扩展对比（3.8）**：

| 声明 | 含义 |
|------|------|
| `int *(*a[10])()` | **数组** `[10]`：元素为「返回 int* 的无参函数指针」 |
| `int (*(*a)[10])()` | **指针** → 指向 `[10]` 数组：元素为「返回 int 的无参函数指针」 |

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch02](../ch02-its-not-a-bug-its-a-language-feature/)** | `*`、`[]` 符号过载（**2.3**）；宏 vs 类型（**2.2**） |
| **[01-K-and-R-C](../../01-K-and-R-C/)** | 基本声明、指针、数组语法 |
| **ch01 1.3** | 预处理器——理解 `#define` 文本替换 |

## 环境

- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch03-analyzing-c-declarations/demo

make all
./demo01_arr_ptr        # char (*Arr5)[5] 数组指针（3.4, 3.8）
./demo02_func_ptr       # typedef 函数指针 MathOp（3.5, 3.8）
./demo03_const_typedef  # const Str 绑定（3.5, 3.6）

# 螺旋例题注释版（只读源码即可）
cat demo04_spiral_examples.c

# 单独编译
gcc -std=c11 -Wall demo01_arr_ptr.c -o demo01_arr_ptr
gcc -std=c11 -Wall demo02_func_ptr.c -o demo02_func_ptr
gcc -std=c11 -Wall demo03_const_typedef.c -o demo03_const_typedef

make clean
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 为何难读** | **3.1** | 编译器文法；`*`/`[]` 组合；必须学螺旋规则 |
| **2 声明结构** | **3.2** | 存储类 + 类型 + declarator；逗号分隔多变量 |
| **3 螺旋规则** | **3.3** | 顺时针；`[]` `()` > `*`；四步算法 |
| **4 图表法** | **3.4** | 框图与螺旋等价；嵌套可视化 |
| **5 typedef** | **3.5–3.7** | PSTR 陷阱；typedef vs `#define`；struct typedef |
| **6 全书例题** | **3.8** | str/p/func/x/a/signal 逐步推演 |
| **7 物理世界** | **3.9** | 声明驱动硬件；类型即契约 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_arr_ptr** | `typedef char (*Arr5)[5]` 数组指针 | **3.4**, **3.8** |
| **demo02_func_ptr** | `typedef int (*MathOp)(int,int)` | **3.5**, **3.8** |
| **demo03_const_typedef** | `const Str` 与 `char*` 区别 | **3.5**, **3.6** |
| **demo04_spiral_examples** | 螺旋规则分步注释（无 main） | **3.3**, **3.8** |

## 高频考点 / 面试题

1. **`char *str[10]` 与 `char (*p)[10]` 区别？** → 前者 **str 是数组**（元素 char*）；后者 **p 是指针**（指向 char[10]）（**3.3**, **3.8**, **demo01**）

2. **`typedef char *PSTR; PSTR a, b;` 中 a、b 类型？** → **都是 char\***；对比 `char *a, b` 仅 a 是指针（**3.5**, **demo03**）

3. **`typedef int x[10]` 与 `#define x int[10]` 声明 `x y, z` 有何不同？** → typedef：y、z 均为 int[10]；宏：展开为 `int[10] y, z` **非法**（**3.6**）

4. **`void (*signal(int sig, void (*handler)(int)))(int)` 含义？** → signal **是函数**：参数为信号号与 `void(int)` 型 handler；**返回** 指向 `void(int)` 的 **函数指针**（旧 handler）。typedef：`typedef void (*SigHandler)(int); SigHandler signal(int, SigHandler);`（**3.8**）

**拓展：**

- `int *(*a[10])()` vs `int (*(*a)[10])()`（**3.8**）
- `char *(*(*x())[])()` 如何分层 typedef（**3.8**）
- `typedef struct foo { } foo;` 三个 foo（**3.7**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[Expert C ch02](../ch02-its-not-a-bug-its-a-language-feature/)** 符号过载与宏；**K&R** 基本语法 |
| 后置 | **[ch04](../ch04-arrays-are-not-pointers/)** 数组 ≠ 指针；**[ch08](../ch08-halloween-vs-christmas/)** 类型转换 |
| 关联 | **[Embedded C ch07](../../05-Embedded-C-Self-Cultivation/ch07-data-storage-and-pointers/)** 指针、存储、MMIO |
| 全书 | **ch05** 链接；**ch07** 内存；函数指针在系统 API 中无处不在 |

## 小节

- [3.1 只有编译器才会喜欢的语法](./3.1-只有编译器才会喜欢的语法.md)
- [3.2 声明是如何形成的](./3.2-声明是如何形成的.md)
- [3.3 优先级规则](./3.3-优先级规则.md)
- [3.4 通过图表分析C语言的声明](./3.4-通过图表分析C语言的声明.md)
- [3.5 typedef可以成为你的朋友](./3.5-typedef可以成为你的朋友.md)
- [3.6 typedef int x[10]和#define x int[10]的区别](./3.6-typedef-int-x-10-和define-x-int-10-的区别.md)
- [3.7 typedef struct foo{ ... foo; }的含义](./3.7-typedef-struct-foo-foo-的含义.md)
- [3.8 理解所有分析过程的代码段](./3.8-理解所有分析过程的代码段.md)
- [3.9 轻松一下——驱动物理实体的软件](./3.9-轻松一下驱动物理实体的软件.md)
