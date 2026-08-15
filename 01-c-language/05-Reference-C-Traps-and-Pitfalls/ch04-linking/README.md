# 第 4 章 连接

**Linking** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch00–ch03](../ch00-introduction/) 侧重 **单文件** 词法/语法/语义；本章进入 **多 `.c` → `.o` → 链接器** 阶段。陷阱多为 **编译通过、链接或运行才暴露**。

```text
  a.c ──► a.o ─┐
  b.c ──► b.o ─┼──► ld ──► a.out
  libfoo.a ────┘      符号解析 + 重定位
```

## 小节索引

| 节 | 主题 |
|----|------|
| [4.1](./4.1-extern声明与定义.md) | `extern` vs 定义、头文件误写 `int g;` |
| [4.2](./4.2-bss与data全局变量.md) | `.bss` / `.data` |
| [4.3](./4.3-static内部链接.md) | `static` 文件内可见 |
| [4.4](./4.4-类型不匹配链接.md) | `arr[]` vs `extern *arr` **静默灾难** |
| [4.5](./4.5-静态库链接顺序.md) | `.a` 左→右扫描 |
| [4.6](./4.6-强弱符号.md) | 多文件 tentative 共享 |
| [4.7](./4.7-未定义引用.md) | 漏 `.o` / 漏库 |
| [4.8](./4.8-头文件保护.md) | include guard |

## 底层开发规范

1. 全局：**头文件 `extern`，单 `.c` 定义**
2. 模块私有：**`static`** 全局与工具函数
3. 静态库：**调用在前，库在后**
4. 头文件：**include guard**，只声明不定义
5. 跨文件符号：**统一头文件类型**
6. 链接后：`nm` / `readelf -s` 核对符号

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch03 语义](../ch03-semantic-pitfalls/) |
| **后置** | [ch05 库函数](../ch05-library-functions/) |
| **交叉** | [Expert C ch05 链接](../../03-Advanced-Expert-C-Programming/ch05-thinking-of-linking/) |

## Demo

```bash
cd demo && make all
./demo01_extern/main
./demo02_extern_type/use_wrong    # 段错误：类型不匹配
./demo02_extern_type/use_correct
./demo03_static_lib/demo
./demo04_bss_data/main            # nm 显示 B/D 段
make -C demo05_undef link_fail      # undefined reference
```

## 面试题

1. `extern int a;` 与 `int a;` 区别？
2. 为何 `extern char *p` 不能对应 `char p[] = "x"`？
3. `static` 全局与 `extern` 全局链接属性？
4. 静态库为何要注意链接顺序？
5. `undefined reference` 与 `multiple definition` 各什么原因？

## 章节自测

> 连接陷阱：编译通过、链接或运行才暴露。看代码 → 想答案 → 点开验证。

### Q1: extern 类型不匹配灾难

```c
// file_a.c
char name[] = "hello world";

// file_b.c
extern char *name;
#include <stdio.h>
int main() {
    printf("%s\n", name);
    return 0;
}
```

> 编译链接能通过吗？运行时发生什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 链接**可能成功**（C 链接器不做类型检查），但运行时**崩溃**。

`file_a.c` 中 `name` 是 `char[12]`（12 字节连续数据）。`file_b.c` 中 `extern char *name` 把它当指针——读取 `name` 的前 8 字节（64 位指针）当作地址值，即把 `"hello wo"` 的字节解释为一个内存地址去解引用 → **SIGSEGV**。

**规则：** 跨文件 `extern` 声明必须与定义**类型完全一致**。`char[]` ≠ `char *`。

**复习：** → [4.4 类型不匹配链接](./4.4-类型不匹配链接.md)

</details>

### Q2: static 隐藏符号

```c
// counter.c
static int count = 0;
int get_count(void) { return ++count; }

// main.c
#include <stdio.h>
extern int count;   // 尝试访问 counter.c 的 count
int main() {
    printf("%d\n", count);
    return 0;
}
```

> 链接结果如何？

<details>
<summary>答案与复习指引</summary>

**答案：** 链接**失败**——`undefined reference to 'count'`。`static` 全局变量是**内部链接**，只在本 `.c` 文件可见，其他文件无法 `extern` 访问。

**用途：** 模块私有变量和工具函数用 `static` 封装，避免命名冲突——内核大量使用。

**复习：** → [4.3 static 内部链接](./4.3-static内部链接.md)

</details>

### Q3: 静态库链接顺序

```c
// liba.a 调用 libb.a 的函数
// main.c 调用 liba.a 的函数

// 命令1：gcc main.o -la -lb   →  ?
// 命令2：gcc main.o -lb -la   →  ?
```

> 两条命令哪个能链接成功？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 命令1成功，命令2**失败**（`undefined reference`）。

静态库（`.a`）链接器从左到右扫描：只提取满足**当前未解析符号**的 `.o`。命令2先扫 `libb.a`，此时没有未解析符号引用 `libb`，所以 `libb` 被跳过；扫到 `liba.a` 时发现需要 `libb` 的符号，但 `libb` 已经过去了。

**规则：** 被依赖的库放右边——`调用者 → 被调用者` 的顺序。

**复习：** → [4.5 静态库链接顺序](./4.5-静态库链接顺序.md)

</details>

### Q4: bss vs data

```c
int a = 42;       // 初始化为非零
int b = 0;        // 初始化为零
int c;            // 未初始化

#include <stdio.h>
int main() {
    printf("%d %d %d\n", a, b, c);
    return 0;
}
```

> `a`、`b`、`c` 分别在哪个段？用 `nm` 查看是什么字母？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `a = 42`：`.data` 段（初始化非零）— `nm` 显示 **D**
- `b = 0`：`.bss` 段（初始化为零）— `nm` 显示 **B**
- `c`（未初始化）：`.bss` 段（C 标准规定未初始化全局变量默认 0）— `nm` 显示 **B**（或 C，取决于编译器）

**区别：** `.data` 占可执行文件空间；`.bss` 不占（运行时由 OS 清零）——大量零初始化全局变量用 `.bss` 省空间。

**复习：** → [4.2 bss 与 data](./4.2-bss与data全局变量.md)

</details>

### Q5: 头文件重复包含

```c
// types.h
struct Point { int x, y; };

// a.h
#include "types.h"

// b.h
#include "types.h"

// main.c
#include "a.h"
#include "b.h"
#include <stdio.h>
int main() { struct Point p = {1, 2}; printf("%d,%d\n", p.x, p.y); }
```

> 编译结果如何？如何修复？

<details>
<summary>答案与复习指引</summary>

**答案：** 编译**失败**——`struct Point` 被重复定义。`types.h` 被通过 `a.h` 和 `b.h` 两次包含。

**修复：** 加 include guard：

```c
#ifndef TYPES_H
#define TYPES_H
struct Point { int x, y; };
#endif
```

或 `#pragma once`（非标准但广泛支持）。

**复习：** → [4.8 头文件保护](./4.8-头文件保护.md)

</details>
