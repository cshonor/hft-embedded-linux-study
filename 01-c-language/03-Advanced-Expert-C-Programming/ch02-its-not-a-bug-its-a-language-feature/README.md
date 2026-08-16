# 第 2 章 这不是 Bug，而是语言特性

**It's Not a Bug, It's a Language Feature** — Peter van der Linden, *Expert C Programming*

## 本章目标

建立 C **「怪行为」分类框架**：区分 **语言特性、UB、实现定义** 与 **无法撤回的历史错误**。掌握 **三类之过**——多做（commission）、误做（omission/overloading）、少做（subtraction）；能解释 `switch` 贯穿、宏替换、符号过载、signed/unsigned 比较等 **面试高频陷阱**；会用 **编译警告与 Sanitizer** 验证；为 **ch03 声明**、**ch04 数组**、**ch07 内存** 的 UB 案例打底。

## 核心思想：三类之过

| 类别 | 英文 | 含义 | 代表特性 | 小节 |
|------|------|------|----------|------|
| **多做之过** | Sins of Commission | 语言 **主动多做** 一步 | `switch` fall-through；字符串字面量拼接；默认 **extern** 链接；隐式 int / K&R 无原型；宏文本替换 | **2.2** |
| **误做之过** | Sins of Omission | **符号过载**、规则不替人想 | `static` 双义；`*` vs `.`/`->`；`=`/`==`/`&`；`*` `&` `[]` 多重含义；非 void 缺 **return** | **2.3** |
| **少做之过** | Sins of Subtraction | 语言 **故意不做** 检查 | 无数组边界；C89 无 bool；字符串无长度；**signed/unsigned 隐式转换** | **2.4** |

**2.1** 用 Fortran DO 循环说明：**符合语法 ≠ 符合意图**。**2.5** 补充 trigraphs、`//` 与 C89 等 **真·历史 Bug**。**2.6** 列参考文献与标准条款索引。

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch01](../ch01-c-through-the-mists-of-time/)** | K&R vs ANSI、UB 分类、宏基础（**1.3–1.6**） |
| **[01-Primer-K-and-R-C](../../01-Primer-K-and-R-C/)** | 基本 C 语法；本章不教语法，教 **陷阱** |
| **Embedded C ch04**（可选） | 链接与符号可见性，理解 **extern/static** |

## 环境

- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra -Wconversion -Wimplicit-fallthrough`
- **Sanitizer**（可选）：`-fsanitize=undefined,address`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/03-Advanced-Expert-C-Programming/ch02-its-not-a-bug-its-a-language-feature/demo

make all
./demo01_switch          # switch fall-through（2.2）
./demo02_sign_unsigned   # unsigned vs signed 比较（2.4）
./demo03_macro_mul       # 宏无括号 MULT（2.2）
./demo04_static_dual     # static 文件 vs 块内（2.3）

# 打开 fall-through 警告
gcc -std=c11 -Wall -Wimplicit-fallthrough demo01_switch.c -o demo01_switch

# 符号比较警告
gcc -std=c11 -Wall -Wsign-compare demo02_sign_unsigned.c -o demo02_sign_unsigned

make clean
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 特性 vs Bug** | **2.1** | Fortran DO；历史妥协；编译器不猜意图 |
| **2 多做之过** | **2.2** | fall-through、字符串拼接、extern、K&R、**MULT/MAX 宏** |
| **3 误做之过** | **2.3** | static 双义、优先级、`= vs ==`、`[]` 声明 |
| **4 少做之过** | **2.4** | 无边界、无 bool、C 字符串、整型转换 |
| **5 真·Bug** | **2.5** | trigraphs、C89 无 `//`、悬空 else |
| **6 文献** | **2.6** | K&R、ISO C、Ritchie、Koenig、WG14 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | `switch` 无 `break` 贯穿 | **2.2** |
| **demo02** | `unsigned` 与 `int` 比较 | **2.4** |
| **demo03** | `MULT(1+2,3+4)` 宏优先级 | **2.2** |
| **demo04** | `static` 文件作用域 vs 块内 | **2.3** |

## 高频考点 / 面试题

1. **`switch` 中漏写 `break` 会怎样？这是 Bug 还是语言特性？** → 特性；匹配 case 后 **贯穿** 执行直到 `break` 或结束（**2.2**，**demo01**）
2. **`unsigned u = 5; int i = -1; if (u > i)` 结果？** → `i` 转为 `unsigned`；条件为假（**2.4**，**demo02**）
3. **`#define MULT(a,b) a*b` 与 `MULT(1+2,3+4)` 的值？** → 展开 `1+2*3+4 = 11`；应 `((a)*(b))`（**2.2**，**demo03**）
4. **`#define MAX(a,b) ((a)>(b)?(a):(b))` 与 `MAX(i++, j++)` 有何问题？** → 副作用 **重复求值** → UB；用 inline 函数（**2.2**）
5. **`static` 在文件顶部与函数内分别表示什么？** → 文件内 **内部链接** vs 块内 **静态存储期局部变量**（**2.3**，**demo04**）

**拓展（书中常问）：**

- 相邻字符串 `"a" "b"` 何时合并？编译期（**2.2**）
- `if (a=b)` 与 `if (a==b)`、`if (a&b)` 区别（**2.3**）
- C 字符串为何易溢出？无长度 + `strcpy` 不检查（**2.4**）
- trigraphs 是什么？为何 C23 删除（**2.5**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[Expert C ch01](../ch01-c-through-the-mists-of-time/)** 标准观与宏；**K&R** 基础语法 |
| 后置 | **[ch03](../ch03-analyzing-c-declarations/)** C 声明读法；**[ch04](../ch04-arrays-are-not-pointers/)** 数组非指针 |
| 关联 | **[Embedded C](../../05-Kernel-Prep-Embedded-C-Self-Cultivation/)** ch04 链接、ch06 GNU 扩展、ch07 指针 |
| 全书 | **ch05** 链接；**ch07** 内存；**ch08** 类型转换 |

## 小节

- [2.1 这关语言特性何事，在Fortran里这就是Bug呀](./2.1-这关语言特性何事-在Fortran里这就是Bug呀.md)
- [2.2 多做之过](./2.2-多做之过.md)
- [2.3 误做之过](./2.3-误做之过.md)
- [2.4 少做之过](./2.4-少做之过.md)
- [2.5 轻松一下——有些特性确实就是Bug](./2.5-轻松一下有些特性确实就是Bug.md)
- [2.6 参考文献](./2.6-参考文献.md)

## 章节自测

> 语言特性 vs Bug：三类之过——多做、误做、少做。看代码 → 想答案 → 点开验证。

### Q1: switch 贯穿

```c
int level = 2;
int points = 0;

switch (level) {
    case 1:
        points += 10;
    case 2:
        points += 20;
    case 3:
        points += 30;
    default:
        points += 5;
}

printf("points = %d\n", points);
```

> `points` 的值是多少？这是 Bug 还是语言特性？

<details>
<summary>答案与复习指引</summary>

**答案：** `points = 55`（20+30+5）。`case 2` 匹配后没有 `break`，**贯穿**执行后续所有 case 直到 `break` 或结束。

**这是语言特性**（fall-through），不是 Bug——但容易引发错误。`-Wimplicit-fallthrough` 可检测意外贯穿。

**用途：** 有意贯穿可以简化代码（如 `case 1: case 2: ...` 共享逻辑）。

**复习：** → [2.2 多做之过](./2.2-多做之过.md)

</details>

### Q2: signed vs unsigned 比较

```c
unsigned int remaining = 10;
int requested = -1;

if (requested <= remaining)
    printf("OK: sending %d items\n", requested);
else
    printf("ERROR: too many\n");
```

> 输出什么？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 输出 `ERROR: too many`。`requested = -1` 与 `unsigned` 比较时，`requested` 被隐式转换为 `unsigned`，变成 `4294967295`（UINT_MAX），远大于 10。

**这是"少做之过"**——C 不阻止 signed/unsigned 混合比较，编译器通常只给 warning。

**HFT 灾难：** 如果用 `int` 表示订单数量，`-1` 作为"无效"标记，与 `unsigned` 库存比较时永远"充足"。

**复习：** → [2.4 少做之过](./2.4-少做之过.md)

</details>

### Q3: 宏优先级陷阱

```c
#define MULT(a, b) a * b

int r1 = MULT(2, 3);        // A
int r2 = MULT(1 + 2, 3);    // B
int r3 = MULT(2, 1 + 2);    // C
```

> r1、r2、r3 分别是多少？

<details>
<summary>答案与复习指引</summary>

**答案：** `r1 = 6`，`r2 = 5`（`1 + 2 * 3 = 7`？不对——展开是 `1 + 2 * 3` = 1+6 = 7），`r3 = 5`（`2 * 1 + 2` = 4？不对——展开是 `2 * 1 + 2` = 2+2 = 4）。

重新计算：
- r2 = `1 + 2 * 3` = 1 + 6 = **7**
- r3 = `2 * 1 + 2` = 2 + 2 = **4**

**修正：** `#define MULT(a, b) ((a) * (b))` — 每个参数和整体都加括号。

**复习：** → [2.2 多做之过](./2.2-多做之过.md)

</details>

### Q4: static 双义

```c
// file scope（文件顶部）
static int counter = 0;       // 含义 A

void func(void) {
    static int call_count = 0; // 含义 B
    call_count++;
    counter++;
}

// 另一个文件
extern int counter;           // 能链接吗？
```

> 含义 A 和 B 分别是什么？extern 能访问 counter 吗？

<details>
<summary>答案与复习指引</summary>

**答案：**
- A（文件作用域 `static`）：**内部链接**——只在本 `.c` 文件可见，`extern` 无法访问。
- B（块作用域 `static`）：**静态存储期**——函数返回后值不丢失，但作用域仍限于函数内。只在首次调用时初始化。

`extern int counter` **链接失败**——`static` 全局变量是内部链接。

**这是"误做之过"**——`static` 一个关键字两种含义，容易混淆。

**复习：** → [2.3 误做之过](./2.3-误做之过.md)

</details>
