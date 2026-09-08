# 第 6 章 预处理器

**Preprocessor** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch05 库函数](../ch05-library-functions/) 之后，本章聚焦 **编译前文本替换**：宏展开、条件编译、头文件防护 —— 预处理器 **不理解 C 类型与优先级**，是内核/底层宏 bug 最高发区。

```text
  .c ──► 预处理器 ──► 翻译单元 ──► 编译 ──► .o
              ↑
         纯文本 #define / #include / #if
```

## 小节索引

| 节 | 主题 |
|----|------|
| [6.1](./6.1-宏参数括号.md) | 参数/整体括号、优先级 |
| [6.2](./6.2-宏副作用重复.md) | `i++` 重复求值 |
| [6.3](./6.3-多行宏与分号.md) | if-else、`do-while(0)` |
| [6.4](./6.4-字符串化与连接.md) | `#`、`##` |
| [6.5](./6.5-宏名冲突.md) | 宏覆盖 typedef/关键字 |
| [6.6](./6.6-头文件防护.md) | `#ifndef` / `#pragma once` |
| [6.7](./6.7-条件编译与注释.md) | 注释无法屏蔽 `#ifdef` |
| [6.8](./6.8-宏无类型检查.md) | `MAX` 混类型 |
| [6.9](./6.9-空宏与defined.md) | `#ifdef` vs `#if DEBUG` |

## 底层宏编写规范

1. 参数与整体 **双层括号**
2. **禁止** 副作用实参传入宏
3. 多语句宏用 **`do { ... } while (0)`**
4. 常量宏 **全大写**；逻辑用 **inline 函数**
5. 头文件 **include guard**
6. 大段禁用代码用 **`#if 0`**，不靠块注释包 `#ifdef`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch05 库函数](../ch05-library-functions/) |
| **后置** | [ch07 可移植性](../ch07-portability-pitfalls/) |
| **交叉** | [ch04 4.8 头文件保护](../ch04-linking/4.8-头文件保护.md) |

## Demo

```bash
cd demo && make all
./demo01_parens/main
./demo02_side_effect/main
./demo03_dowhile/main
./demo04_stringify/main
./demo05_token_paste/main
make -C demo06_cond_compile debug=1
./demo07_max/main
```

## 面试题

1. `#define SQUARE(x) x*x` 与 `((x)*(x))` 区别？
2. 为何 `SQUARE(i++)` 危险？inline 如何解决？
3. `do-while(0)` 宏解决什么问题？
4. `#` 与 `##` 各做什么？
5. `#ifdef DEBUG` 与 `#if DEBUG` 区别？

## 章节自测

> 预处理器陷阱：纯文本替换，不理解类型和优先级。看代码 → 想答案 → 点开验证。

### Q1: 宏括号缺失

```c
#define SQUARE(x) x*x

int r1 = SQUARE(3);
int r2 = SQUARE(3 + 1);
int r3 = SQUARE(2 + 3);
```

> `r1`、`r2`、`r3` 分别是多少？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `r1 = 9`（`3*3`）
- `r2 = 7`（`3+1*3+1` = 3+3+1 = 7，**不是 16**）
- `r3 = 11`（`2+3*2+3` = 2+6+3 = 11，**不是 25**）

**修正：** `#define SQUARE(x) ((x)*(x))` — 参数和整体都加括号。

**复习：** → [6.1 宏参数括号](./6.1-宏参数括号.md)

</details>

### Q2: 副作用重复求值

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int i = 3, j = 5;
int m = MAX(i++, j++);
printf("m=%d i=%d j=%d\n", m, i, j);
```

> `m`、`i`、`j` 分别是多少？

<details>
<summary>答案与复习指引</summary>

**答案：** `m=6, i=4, j=7`（或类似值，具体取决于求值顺序）。

展开：`((i++) > (j++) ? (i++) : (j++))`。`i++`=3 vs `j++`=5，3 < 5 为真，走 `j++` 分支 → `j` 被**自增两次**（6→7），`i` 自增一次（3→4）。`m` = 第二次 `j++` 的值 = 6。

**UB 警告：** 三目运算符的 condition 和选中分支之间的求值顺序是 sequenced 的，但 `i++`/`j++` 各被执行了不确定次数——行为不可靠。

**修正：** 用 `inline` 函数替代宏；或用 GNU 语句表达式 `({ typeof(a) _a=(a); ... })`。

**复习：** → [6.2 宏副作用重复](./6.2-宏副作用重复.md)

</details>

### Q3: do-while(0) 宏

```c
#define SAFE_FREE(p) \
    if (p) { free(p); p = NULL; }

int main() {
    char *a = malloc(10), *b = malloc(10);
    if (1)
        SAFE_FREE(a);
    else
        SAFE_FREE(b);    // 会被执行吗？
    return 0;
}
```

> `else` 分支的 `SAFE_FREE(b)` 会如预期执行吗？如何修复？

<details>
<summary>答案与复习指引</summary>

**答案：** **不会**如预期。展开后：

```c
if (1)
    if (a) { free(a); a = NULL; }
else
    if (b) { free(b); b = NULL; }
```

`else` 绑定内层 `if (a)`，而非外层 `if (1)`——**悬垂 else 问题**。

**修复：** 用 `do { ... } while(0)` 包裹宏：

```c
#define SAFE_FREE(p) do { if (p) { free(p); p = NULL; } } while(0)
```

`do-while(0)` 保证宏体是一个语句，不受外层 `if-else` 影响。

**复习：** → [6.3 多行宏与分号](./6.3-多行宏与分号.md)

</details>

### Q4: 字符串化 #

```c
#define STR(x) #x
#define XSTR(x) STR(x)

#define VERSION 42

const char *a = STR(VERSION);
const char *b = XSTR(VERSION);
```

> `a` 和 `b` 分别是什么字符串？

<details>
<summary>答案与复习指引</summary>

**答案：** `a = "VERSION"`（`#` 在宏参数展开**之前**字符串化）；`b = "42"`（`XSTR` 的参数先展开 `VERSION` → `42`，再传给 `STR` 字符串化）。

**用途：** 调试宏中打印表达式和值：`printf("%s = %d\n", STR(x), x);`

**复习：** → [6.4 字符串化与连接](./6.4-字符串化与连接.md)

</details>

### Q5: #ifdef vs #if

```c
#define DEBUG 0
#define RELEASE

#ifdef DEBUG
    int debug_mode = 1;    // A
#endif

#if DEBUG
    int debug_verbose = 1; // B
#endif

#ifdef RELEASE
    int release_mode = 1;  // C
#endif
```

> A、B、C 三行哪些会被编译？

<details>
<summary>答案与复习指引</summary>

**答案：** A 和 C 编译，B **不编译**。

- `#ifdef DEBUG`：检查 `DEBUG` **是否被定义**（不管值是多少）——`DEBUG` 定义为 0，仍然"已定义" → A 编译。
- `#if DEBUG`：检查 `DEBUG` 的**值**是否非零——`DEBUG` = 0 → B 不编译。
- `#ifdef RELEASE`：`RELEASE` 已定义（空宏）→ C 编译。

**规则：** 想用 0/1 开关用 `#if`；只想检查是否定义用 `#ifdef`。

**复习：** → [6.9 空宏与 defined](./6.9-空宏与defined.md)

</details>
