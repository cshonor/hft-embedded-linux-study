# Ch11 · Pointers（指针）

> **Level 2 · 相知** · 策略：**🟡 对照速读**（《C 和指针》已深讲）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

`&`/`*` 操作符、指针算术、指针与结构体、不透明结构、数组与指针等价性、空指针、函数指针。
《C 和指针》全书已深入覆盖；本章只看两处 **C23 增量**和**不透明结构封装模式**。

## 一、指针基础（速过，详见《C 和指针》）

```c
int x = 42;
int *p = &x;       // p 存储 x 的地址
*p = 100;          // 通过 p 修改 x → x 变为 100
int **pp = &p;     // pp 存储 p 的地址（二级指针）
```

| 关键概念 | 说明 |
|----------|------|
| 指针 = 地址 + 类型 | 地址决定"从哪开始读"，类型决定"读多少字节、怎么解释" |
| `&` 取地址 | 获取变量的内存地址 |
| `*` 解引用 | 通过地址访问那块内存 |
| 指针算术 | `p + 1` 前进 `sizeof(*p)` 字节（指针缩放） |
| 数组退化 | 数组名在表达式中退化为指向首元素的指针 |

> 详见 [K&R 5.1–5.4](../../01-Primer-K-and-R-C/ch05-pointers-and-arrays/5.1-指针与地址.md) 和《C 和指针》全书。

## 二、不透明结构（Opaque Struct）— 工程重点

### 基本模式

```c
/* 头文件 foo.h */
struct foo;                       /* 前向声明，不暴露内部 */
typedef struct foo foo_t;

foo_t *foo_create(int param);
void   foo_destroy(foo_t *f);
int    foo_process(foo_t *f, const void *in, size_t len);
```

```c
/* 实现文件 foo.c */
#include "foo.h"
#include <stdlib.h>

struct foo {                      /* 真正的布局，只有 .c 知道 */
    int  param;
    void *internal_buf;
    size_t buf_size;
};

foo_t *foo_create(int param) {
    foo_t *f = malloc(sizeof(*f));
    if (!f) return NULL;
    f->param = param;
    f->buf_size = 1024;
    f->internal_buf = malloc(f->buf_size);
    if (!f->internal_buf) { free(f); return NULL; }
    return f;
}
```

### 为什么不透明结构重要

| 好处 | 说明 |
|------|------|
| 封装 | 用户无法直接访问内部成员 |
| ABI 稳定 | 改内部布局不破坏二进制兼容 |
| 减少编译依赖 | 改 `.c` 不触发用户重编译 |
| 安全 | 用户不能绕过接口直接修改状态 |

> **DPDK 的 `rte_ring`、`rte_mempool`、`rte_hash` 都是这个模式。** 对外只暴露 `struct rte_ring *`，内部布局在 `rte_ring_core.h`（不直接被用户包含）。

## 三、C23 空指针：`nullptr`

| 标准 | 写法 | 说明 |
|------|------|------|
| C89 | `NULL` | 通常 `(void *)0` 或 `0` |
| C99+ | `NULL` | 同上，`<stddef.h>` 定义 |
| **C23** | `nullptr` | **真正的关键字**，类型为 `nullptr_t` |

```c
/* C23 之前 */
int *p = NULL;         // NULL 可能是 ((void *)0) 或 0
int *q = 0;            // 合法但不推荐

/* C23 */
int *p = nullptr;      // ✅ 清晰、类型安全
```

### `nullptr` 解决的问题

```c
/* C89 的 NULL 歧义 */
void f(int x);         // 重载1（C++ 才有重载，但 C 的可变参数也有问题）
void f(char *p);

f(NULL);    // ⚠ NULL 可能是 0（int）→ 调用 f(int)！
f(nullptr); // ✅ nullptr 类型是 nullptr_t → 调用 f(char *)

/* 函数指针比较 */
int (*fp)(int) = NULL;
if (fp == NULL) { }      // OK，但 NULL 可能是 0
if (fp == nullptr) { }   // ✅ C23：类型更明确
```

| `nullptr` 优势 | 说明 |
|----------------|------|
| 类型安全 | `nullptr` 类型是 `nullptr_t`，可隐式转换为任何指针类型 |
| 消除歧义 | 在可变参数函数和 C++ 重载中不会误匹配 `int` |
| 可读性 | `nullptr` 比 `NULL` 或 `0` 更明显是"空指针" |

> 迁移建议：C23 项目逐步用 `nullptr` 替代 `NULL`；C11 项目继续用 `NULL`。

## 四、函数指针

```c
/* 函数指针声明 */
int (*compare)(const void *, const void *);

/* typedef 简化 */
typedef int (*compare_fn)(const void *, const void *);
compare_fn cmp = cmp_int;

/* qsort */
qsort(arr, n, sizeof(int), cmp);
```

### 函数指针数组（跳转表）

```c
/* HFT 消息分发：用函数指针数组替代 switch */
typedef int (*handler_fn)(struct msg *);

static handler_fn handlers[MSG_TYPE_MAX] = {
    [MSG_NEW_ORDER]   = handle_new_order,
    [MSG_CANCEL]      = handle_cancel,
    [MSG_MODIFY]      = handle_modify,
    [MSG_HEARTBEAT]   = handle_heartbeat,
};

int dispatch(struct msg *msg) {
    if (msg->type >= MSG_TYPE_MAX || !handlers[msg->type])
        return -1;
    return handlers[msg->type](msg);
}
```

> 跳转表比 `switch` 更快（O(1) 直接索引，无分支预测）；DPDK 收包路径大量使用此模式。

### C23 函数指针属性

```c
/* C23：函数指针可以带属性 */
[[noreturn]] (*error_handler)(const char *);

/* 设置 noreturn 的错误处理函数 */
error_handler = fatal_exit;
```

## 五、`restrict` 限定符（速过，详见 Ch16）

```c
/* restrict：承诺指针不与其它指针别名 */
void memcpy(void *restrict dest, const void *restrict src, size_t n);
```

> 详见 [Ch16 性能](../ch16-performance/README.md) 的 restrict 专题。

## 六、指针与 const

```c
const int *p;          // 指向 const int 的指针：不能通过 p 修改数据
int *const q = &x;     // const 指针：不能修改 q 本身（指向固定）
const int *const r = &x;  // 都不能改

/* 口诀：const 在 * 左边修饰数据，在 * 右边修饰指针 */
```

## HFT / DPDK 关联

| 特性 | HFT 用途 |
|------|----------|
| 不透明结构 | DPDK `rte_ring`/`rte_mempool`/`rte_hash` 的封装模式 |
| `nullptr` | C23 后替代 `NULL`，消除歧义 |
| 函数指针数组 | 消息分发跳转表（比 switch 快） |
| `restrict` | 热路径函数别名消除（详见 Ch16） |
| `const` 正确性 | 不修改的参数加 `const`，线程安全信号 |

## 自测题

<details><summary>1. 不透明结构模式中，用户为什么不能 <code>sizeof(struct foo)</code>？</summary>

因为头文件只有前向声明 `struct foo;`，没有完整定义。编译器不知道 `struct foo` 的布局，
无法计算大小。用户只能通过 `foo_create()` 获取指针，所有操作通过接口函数完成。
`sizeof(struct foo *)` 是可以的（指针大小已知）。
</details>

<details><summary>2. C23 的 <code>nullptr</code> 和 <code>NULL</code> 有什么本质区别？</summary>

`NULL` 是宏，可能是 `((void *)0)` 或 `0`——在不同实现中类型不同，可能在可变参数函数或
C++ 重载中产生歧义。`nullptr` 是 C23 关键字，类型为 `nullptr_t`，可隐式转换为任何指针类型，
但不会转换为 `int`，彻底消除歧义。
</details>

<details><summary>3. 函数指针数组（跳转表）比 <code>switch</code> 有什么优势？</summary>

跳转表是 O(1) 的直接索引访问——`handlers[type](msg)` 一次内存读取 + 一次间接调用，
没有分支预测失败。`switch` 如果 case 稠密会被编译器优化为跳转表，但如果 case 稀疏
或运行时才知道，跳转表更可控。HFT 消息分发路径用跳转表保证确定性延迟。
</details>
