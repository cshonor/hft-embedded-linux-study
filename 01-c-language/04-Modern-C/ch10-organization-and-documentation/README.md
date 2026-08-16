# Ch10 · Organization and documentation（组织与文档）

> **Level 2 · 相知** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

接口设计（头文件）、实现隐藏、宏的使用边界、纯函数。这些是工程组织的"软技能"，
K&R 和前三本书讲得少，但对写 DPDK 库接口直接有用。

## 一、接口设计原则

### 好接口的特征

| 原则 | 说明 | 例子 |
|------|------|------|
| **最小暴露** | 头文件只暴露用户必须知道的 | `struct ring;`（不透明），不暴露内部布局 |
| **命名一致** | 同一模块的函数有统一前缀 | `ring_create` / `ring_enqueue` / `ring_dequeue` |
| **错误处理统一** | 返回值约定一致 | DPDK: 0 成功，负数错误码 |
| **参数顺序一致** | 同类函数参数顺序相同 | DPDK: `(struct rte_ring *r, void *obj, ...)` |
| **不暴露实现细节** | 用户代码不依赖内部数据结构 | 改 `struct ring` 布局不触发用户重编译 |

### 不透明结构封装模式

```c
/* ===== ring.h：公共接口 ===== */
#ifndef RING_H
#define RING_H

#include <stdint.h>
#include <stddef.h>

/* 不透明结构：只前向声明，不暴露内部 */
typedef struct ring ring_t;

/* 创建/销毁 */
ring_t *ring_create(uint32_t capacity);
void    ring_destroy(ring_t *r);

/* 操作 */
int   ring_enqueue(ring_t *r, const void *item);
void *ring_dequeue(ring_t *r);
size_t ring_count(const ring_t *r);

#endif /* RING_H */
```

```c
/* ===== ring.c：实现（用户看不到） ===== */
#include "ring.h"
#include <stdlib.h>
#include <stdatomic.h>

struct ring {
    _Alignas(64) _Atomic uint32_t head;    // 生产者索引（独占缓存行）
    _Alignas(64) _Atomic uint32_t tail;    // 消费者索引（独占缓存行）
    uint32_t mask;
    void    *slots[];
};

ring_t *ring_create(uint32_t capacity) {
    /* capacity 必须 2 的幂 */
    if (capacity & (capacity - 1)) return NULL;
    ring_t *r = malloc(sizeof(ring_t) + capacity * sizeof(void *));
    if (!r) return NULL;
    atomic_store(&r->head, 0);
    atomic_store(&r->tail, 0);
    r->mask = capacity - 1;
    return r;
}
// ...
```

> **这就是 DPDK `rte_ring` 的设计模式**。用户只拿到 `struct rte_ring *` 句柄，内部布局在 `rte_ring_core.h` 中，不对外暴露。

## 二、实现隐藏的好处

| 好处 | 说明 |
|------|------|
| **ABI 稳定** | 改内部结构布局不破坏二进制兼容（用户不依赖 sizeof/offsetof） |
| **减少重编译** | 改 `.c` 文件只需重新编译该文件 + 重链接，不需要重编译所有依赖者 |
| **封装** | 用户无法直接访问内部成员，保证不变量 |

### C23 增强：`[[maybe_unused]]` 和 `[[deprecated]]`

```c
/* 条件编译时有些参数可能未使用 */
void log_msg(int level, const char *msg [[maybe_unused]]) {
#ifdef ENABLE_LOGGING
    if (level >= threshold) printf("%s\n", msg);
#endif
}

/* 标记弃用接口 */
[[deprecated("use ring_enqueue_batch instead")]]
int ring_enqueue(ring_t *r, const void *item);
```

## 三、宏的使用边界

```c
/* ✅ 宏适合：条件编译、字符串拼接、平台抽象 */
#ifdef NDEBUG
#define LOG(...) ((void)0)
#else
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#endif

#define RING_NAME(r) (#r)   // 字符化

/* ❌ 宏不适合：可以用函数替代的逻辑 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))   // 有副作用陷阱！

/* ✅ C23 替代：static inline + typeof 或 _Generic */
static inline int max_int(int a, int b) { return a > b ? a : b; }

/* ✅ C23 泛型 */
#define max(a, b) _Generic((a), \
    int:    max_int, \
    double: max_double, \
    default: max_int \
)((a), (b))
```

> 详见 [Ch17 类函数宏](../ch17-function-like-macros/README.md) 和 [Ch18 类型泛型](../ch18-type-generic-programming/README.md)。

## 四、纯函数

**纯函数** (pure function)：输出只依赖输入，没有副作用。Modern C 鼓励纯函数风格。

```c
/* ✅ 纯函数：给定输入总有相同输出，不修改任何状态 */
uint32_t hash_order(const struct order *o) {
    return o->id * 2654435761u;   // Knuth 乘法哈希
}

/* ⚠️ 非纯函数：依赖全局状态 */
uint32_t hash_with_counter(const struct order *o) {
    static uint32_t salt = 0;
    salt++;                        // 副作用：修改全局状态
    return (o->id * 2654435761u) ^ salt;
}
```

| 纯函数好处 | 说明 |
|------------|------|
| **可测试** | 不需要 mock 全局状态 |
| **线程安全** | 没有共享可变状态 |
| **可并行** | 编译器可自由重排/向量化 |
| **可缓存** | 相同输入缓存结果（memoization） |

> HFT 热路径尽量写纯函数——编译器能做更激进的优化（`restrict` + 纯函数 = 自动向量化）。

## HFT / DPDK 关联

| 原则 | DPDK 实践 |
|------|----------|
| 不透明结构 | `rte_ring`、`rte_mempool`、`rte_mbuf`（部分） |
| 命名前缀 | `rte_eth_`、`rte_ring_`、`rte_hash_` |
| 错误码 | 0 成功，负数 = `-errno` |
| `static inline` | 头文件大量内联函数（`rte_ring_enqueue` 等） |
| 纯函数 | 哈希、CRC、校验和计算 |

## 自测题

<details><summary>1. 不透明结构（opaque struct）如何实现 ABI 稳定？</summary>

头文件只前向声明 `struct ring;`，用户只能拿到 `struct ring *` 指针。结构体的真正定义在 `.c` 文件中，
用户无法 `sizeof(struct ring)` 或访问成员。当内部布局改变（加字段、改对齐），用户代码不需要重新编译——
因为用户代码只操作指针，不依赖内部布局。只有动态库的 `.so` 需要更新，ABI 保持兼容。
</details>

<details><summary>2. 什么情况下宏比函数更合适？</summary>

① 条件编译（`#ifdef DEBUG`）；② 字符串操作（`#`、`##`）；③ 编译期字符串拼接（`__FILE__`、`__LINE__`）；
④ 平台/编译器抽象（`#define barrier() __asm__ volatile("" ::: "memory")`）。
如果逻辑可以用 `static inline` 函数表达，就不要用宏——函数有类型检查，宏没有。
</details>

<details><summary>3. 纯函数对编译器优化有什么帮助？</summary>

纯函数的输出只依赖输入，没有副作用——编译器可以：① 自由重排调用顺序；② 删除结果未使用的调用（死代码消除）；
③ 公共子表达式消除（相同输入只算一次）；④ 自动向量化。加上 `restrict` 限定符，编译器还能假设没有别名，
做更激进的内存访问优化。HFT 热路径应尽量写纯函数。
</details>
