# Ch2 · The principal structure of a program（程序的主要结构）

> **Level 0 · 邂逅** · 策略：**⏭️ 跳过**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

注释、`main` 的特殊性、声明 vs 定义、翻译单元与头文件组织。K&R Ch1/4 已覆盖基本概念；
这里只关注 C23 增量和现代头文件设计原则。

## 一、注释

| 写法 | 标准 | 说明 |
|------|------|------|
| `/* ... */` | C89 | 块注释，不能嵌套 |
| `// ...` | C99+ | 行注释，前三本书里 K&R 不用但现代代码标配 |

**C23 新增：** 无新增注释语法，但 `//` 在 C23 中是完全一等公民。

```c
/* 块注释：函数级说明 */
// 行注释：单行备注

/* 注意：块注释不能嵌套
   /* 这样写会报错 */
*/
```

> 内核风格：函数开头用 `/* ... */` 块注释（Documentation/process/coding-style 规定）。

## 二、`main` 的特殊性

```c
/* 标准规定的两种合法签名 */
int main(void) { ... }                       /* 无参数 */
int main(int argc, char *argv[]) { ... }     /* 带命令行参数 */
```

| 要点 | 说明 |
|------|------|
| 返回类型 | `int`，不是 `void`（C23 起可以省略返回类型，默认 `int`） |
| `return 0;` | 表示成功；C99 起可省略（`main` 隐式返回 0） |
| `argc` | 参数个数（含程序名，最小为 1） |
| `argv` | 参数数组，`argv[argc]` 保证为 `NULL` |

**C23 变化：** `main` 可以省略返回类型（隐式 `int` 回归），但**不推荐**——显式写 `int main(void)` 更清晰。

```c
/* C23 合法但不推荐 */
main(void) { /* 隐式返回 int */ }

/* 推荐写法 */
int main(void) { return 0; }
```

> HFT 进程的 `main` 通常做：解析参数 → 初始化 DPDK (`rte_eal_init`) → 绑核 → 启动 lcore 线程 → join。

## 三、声明 vs 定义

| 概念 | 作用 | 例子 |
|------|------|------|
| **声明** (declaration) | 告诉编译器名字存在及其类型 | `extern int x;` / `int f(int);` |
| **定义** (definition) | 实际分配存储 / 提供函数体 | `int x = 42;` / `int f(int n) { return n; }` |
| ** tentative definition** | C 特有：文件级 `int x;` 不算定义，可能合并 | `int x;`（文件作用域） |

```c
/* 头文件 foo.h */
int foo(int);           /* 声明：可出现多次 */

/* 源文件 foo.c */
int foo(int n) {        /* 定义：只能出现一次 */
    return n + 1;
}

int counter;            /* tentative definition：C 特有 */
                        /* 等价于 int counter = 0; */
```

** tentative definition 是 C 独有的坑**：C++ 里 `int x;`（文件级）就是定义，多次包含会链接报错。
HFT 项目混用 C/C++ 时此处容易出问题。

## 四、翻译单元与头文件

**翻译单元** (translation unit) = 预处理后的单个 `.c` 文件（`#include` 已展开）。

```
foo.h  ──┐
         ├──> 预处理器 ──> 翻译单元 foo.o
foo.c  ──┘
```

### 头文件设计原则

```c
/* ring.h —— 只放声明，不放定义 */
#ifndef RING_H          /* include guard */
#define RING_H

#include <stdint.h>

struct ring;            /* 不透明结构：只声明，不暴露内部 */

struct ring *ring_create(uint32_t capacity);
int ring_enqueue(struct ring *r, void *item);
void *ring_dequeue(struct ring *r);

#endif /* RING_H */
```

```c
/* ring.c —— 实现细节，对外不可见 */
#include "ring.h"
#include <stdlib.h>

struct ring {           /* 真正的定义，只有 .c 知道布局 */
    uint32_t head;
    uint32_t tail;
    uint32_t mask;
    void *slots[];
};

struct ring *ring_create(uint32_t capacity) { ... }
```

| 原则 | 理由 |
|------|------|
| 头文件只放声明 | 避免重复定义链接错误 |
| include guard（或 `#pragma once`） | 防止重复包含 |
| 不透明结构（opaque struct） | 隐藏实现细节，改变布局不触发重编译 |
| 最小包含 | 头文件只 `#include` 它自身需要的头 |

> **DPDK 的 `rte_ring.h` 就是这个模式的工业级实现**——对外只暴露 `struct rte_ring *`，内部布局藏在 `rte_ring_core.h` 中。详见 [Ch11 指针](../ch11-pointers/README.md)。

## HFT / DPDK 关联

头文件组织原则对写 DPDK 库接口有直接参考价值：
- 不透明结构封装 → ABI 稳定（改内部不影响使用者）
- `static inline` 函数放头文件 → 零调用开销（见 [Ch16 性能](../ch16-performance/README.md)）
- `#include` 最小化 → 加快编译、减少耦合

## 自测题

<details><summary>1. <code>int x;</code> 在文件作用域和块作用域有什么区别？</summary>

**文件作用域**（全局）：tentative definition，初值为 0，可以被多个翻译单元合并（如果有 `extern` 声明）。
**块作用域**（局部）：未初始化的自动变量，值是垃圾（indeterminate value），必须先赋值再读。
</details>

<details><summary>2. 为什么头文件里不要放函数定义（<code>static inline</code> 除外）？</summary>

头文件被多个 `.c` 包含后，每个翻译单元都会有一份定义 → 链接时"multiple definition"报错。
`static inline` 是例外：`static` 使符号文件内可见，`inline` 提示编译器内联展开，多份副本不冲突。
</details>

<details><summary>3. 不透明结构（opaque struct）怎么实现？有什么好处？</summary>

头文件只写前向声明 `struct foo;`，用户只能拿到 `struct foo *` 指针；结构体的真正定义放在 `.c` 文件里。
好处：① 改变内部布局不触发使用者重编译（ABI 稳定）；② 用户无法直接访问成员，保证封装。
DPDK `rte_ring`、`rte_mempool` 等都是这个模式。
</details>
