# P2.5 — C 系统编程工具箱（GNU C 桥梁）

> 用纯 C 实现内核/HFT 代码里天天用、但标准 C 教材不讲的宏和数据结构。
> 做完这个，读内核源码不再被 `container_of` / `list_head` / `__attribute__` 卡住。

---

## 核心理念（先读这段再动手）

跟 P2 一样——**项目驱动，笔记当字典**。不要先读完 01/04 的 ch06 再开做。
翻一眼标题知道"有这么个东西"，直接写代码，卡住了回来查。

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [6.4.3 container_of 宏](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.4-typeof-container-of/6.4.3-Linux内核中的container_of宏.md) | 从成员指针反推宿主结构体指针 |
| [6.3 语句表达式](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.3-statement-expr/6.3.2-语句表达式.md) | `({ ... })` 让宏里能写多行语句 |
| [6.7.1 aligned](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.7-aligned/6.7.1-地址对齐-aligned.md) | `__attribute__((aligned(64)))` 控制对齐 |
| [6.9 weak 符号](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.9-weak/6.9-属性声明-weak.md) | 弱符号 = 可被覆盖的默认实现 |

---

## 项目目标

把 01-c-language 书 02（C 和指针）+ 书 04（嵌入式自我修养 GNU C）的技能点变成可复用代码库。
P2 练标准 C 够了，这个项目专门补 **GNU C 扩展 + 内核级数据结构模式**——P4 内核模块直接复用。

## 最小预备

```bash
# 在 WSL 里建项目骨架
mkdir -p ~/p2.5-c-toolkit/src ~/p2.5-c-toolkit/tests
cd ~/p2.5-c-toolkit
# 验证 gcc 能用 GNU 扩展
echo 'int main() { int x = ({ int y = 5; y + 1; }); return x - 6; }' | gcc -std=gnu11 -x c - -o /dev/null && echo "GNU C OK"
```

---

## 交付物 1：container_of 宏

### 做什么

实现 Linux 内核的 `container_of(ptr, type, member)` 宏——给定结构体成员的指针，反推出整个结构体的首地址。

### 为什么重要

内核里到处是 `container_of`：`list_entry`、`hlist_entry`、驱动里从子系统回调指针取回私有数据。不懂这个，内核代码一行都读不动。

### 代码骨架

```c
// src/container_of.h
#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>  // offsetof

// 第一步：先写一个"不用 typeof"的版本，理解原理
#define container_of_simple(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// 第二步：加 typeof 做类型检查（内核版）
#define container_of(ptr, type, member) ({          \
    void *__mptr = (void *)(ptr);                    \
    _Static_assert(                                  \
        __builtin_types_compatible_p(                \
            typeof(*(ptr)), typeof(((type *)0)->member)), \
        "type mismatch in container_of()");          \
    ((type *)(__mptr - offsetof(type, member)));     \
})

#endif
```

### 分步实现

1. **先写 `container_of_simple`**：纯指针运算。`offsetof(type, member)` 是标准库宏，算成员在结构体里的偏移。用 `(char *)` 做字节级减法。
2. **写测试**：定义 `struct task { int id; struct list_head list; int priority; }`，取 `&task.list`，用 `container_of` 取回 `task`，验证地址一致。
3. **加 `typeof` 版本**：`typeof(*(ptr))` 拿到成员的类型，`_Static_assert` + `__builtin_types_compatible_p` 做编译期类型检查——传错类型直接编译报错。
4. **故意传错类型试一下**：`container_of(&task.priority, struct task, list)` 应该编译失败。

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 忘了 `(char *)` 转换 | 指针偏移算错 | `void *` 不能做指针运算，`int *` 减法按 sizeof(int) 跳 |
| `typeof` 写成 `typeof(ptr)` | 类型检查不匹配 | 要取 `typeof(*(ptr))`——解引用拿成员类型 |
| 传了 `const` 指针 | 编译警告丢 const | 内核版有 `container_of_const` 变体处理 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| offsetof 怎么算的 | [6.4.4 container_of 实现分析](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.4-typeof-container-of/6.4.4-container_of宏实现分析.md) |
| typeof 语法 | [6.4.1 typeof 关键字](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.4-typeof-container-of/6.4.1-typeof关键字.md) |
| `({ })` 语句表达式 | [6.3.2 语句表达式](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.3-statement-expr/6.3.2-语句表达式.md) |

---

## 交付物 2：侵入式双向链表

### 做什么

实现 Linux `list.h` 风格的侵入式链表：`list_head` 嵌入到任意结构体里，一套链表操作通用。

### 为什么重要

侵入式链表 vs 传统链表的关键区别：链表节点**不持有数据**，而是**嵌入数据内部**。这样同一个结构体可以同时挂在多条链表上，且链表操作代码与数据类型无关。

### 代码骨架

```c
// src/list.h
#ifndef LIST_H
#define LIST_H

#include "container_of.h"

struct list_head {
    struct list_head *next, *prev;
};

// 初始化：prev/next 都指向自己 = 空链表
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void list_init(struct list_head *head) {
    head->next = head;
    head->prev = head;
}

// 头插
static inline void list_add(struct list_head *new, struct list_head *head) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

// 删除
static inline void list_del(struct list_head *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = NULL;
    entry->prev = NULL;
}

// 遍历
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

// 取回宿主结构体
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

// 遍历并取宿主
#define list_for_each_entry(pos, head, member)                \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                              \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
```

### 分步实现

1. **写 `struct list_head` + `list_init` + `list_add` + `list_del`**
2. **画图验证**：纸上画 3 个节点的插入/删除过程，确认指针操作正确（这是 P2 里 C和指针 ch12 的内容）
3. **写测试**：定义 `struct student { int id; struct list_head list; }`，创建 3 个学生，用 `list_add` 挂到链表上，用 `list_for_each_entry` 遍历打印
4. **验证侵入式优势**：给 `student` 加第二个 `struct list_head grade_list`，同时挂到两条链表上

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 忘了初始化 head | 段错误 | `next`/`prev` 是野指针 |
| `list_del` 后还遍历 | use-after-free | 删了节点但循环还在走它的 `next` |
| `list_for_each_entry` 宏写错 | 编译错误或死循环 | `typeof(*pos)` 要解引用指针取类型 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 双链表插入/删除指针操作 | [12.3 双链表](../../01-c-language/02-Pointers-on-C/ch12-using-structures-and-pointers/12.3-doubly-linked-lists/12.3-双链表.md) |
| 函数指针/回调 | [13.3 函数指针](../../01-c-language/02-Pointers-on-C/ch13-advanced-pointer-topics/13.3-function-pointers/13.3-函数指针.md) |

---

## 交付物 3：SPSC 无锁环缓冲

### 做什么

单生产者单消费者（SPSC）无锁 ring buffer，用原子索引实现，缓存行对齐防伪共享。

### 为什么重要

这是 HFT 和内核的核心数据结构——DPDK rte_ring、Linux kfifo 都是 SPSC/MPSC ring。P8 撮合引擎的行情入队直接用这个。

### 代码骨架

```c
// src/ringbuf.h
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

// 缓存行对齐：head 和 tail 分到不同缓存行，防伪共享
struct ringbuf {
    uint8_t *buffer;
    size_t capacity;   // 必须是 2 的幂
    size_t elem_size;
    // 分开两个 cache line
    _Atomic(size_t) head __attribute__((aligned(64)));  // 生产者写
    char _pad1[64 - sizeof(atomic_size_t)];
    _Atomic(size_t) tail __attribute__((aligned(64)));  // 消费者写
    char _pad2[64 - sizeof(atomic_size_t)];
};

int ringbuf_init(struct ringbuf *rb, size_t capacity, size_t elem_size);
void ringbuf_destroy(struct ringbuf *rb);

// 入队（生产者调用）。成功返回 0，满返回 -1
static inline int ringbuf_push(struct ringbuf *rb, const void *data) {
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t next = (head + 1) & (rb->capacity - 1);  // 环绕，capacity 是 2 的幂
    if (next == atomic_load_explicit(&rb->tail, memory_order_acquire)) {
        return -1;  // 满
    }
    memcpy(rb->buffer + head * rb->elem_size, data, rb->elem_size);
    atomic_store_explicit(&rb->head, next, memory_order_release);
    return 0;
}

// 出队（消费者调用）。成功返回 0，空返回 -1
static inline int ringbuf_pop(struct ringbuf *rb, void *data) {
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    if (tail == atomic_load_explicit(&rb->head, memory_order_acquire)) {
        return -1;  // 空
    }
    memcpy(data, rb->buffer + tail * rb->elem_size, rb->elem_size);
    atomic_store_explicit(&rb->tail, (tail + 1) & (rb->capacity - 1), memory_order_release);
    return 0;
}

#endif
```

### 分步实现

1. **先写非原子版**：普通 `int head, tail`，单线程测试逻辑正确
2. **改成 `_Atomic` + memory_order**：生产者 `release` 写 head，消费者 `acquire` 读 head——保证数据写入对消费者可见
3. **加 `aligned(64)`**：head 和 tail 分到不同缓存行。不加的话它们在同一缓存行，生产者和消费者交替写同一个缓存行 = 伪共享 = 性能暴跌
4. **测试**：两个线程，一个猛 push，一个猛 pop，跑 1000 万次，验证无丢无重
5. **性能对比**：去掉 `aligned(64)` 再跑一遍，用 `perf stat` 看 cache-miss 差异

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| capacity 不是 2 的幂 | 环绕算错 | `& (cap-1)` 只在 2 的幂时等价于 `% cap` |
| memory_order 用错 | 偶尔丢数据 | push 必须 release，pop 必须 acquire |
| 没加 aligned | 性能差 5-10x | 伪共享导致缓存行在核间反复弹跳 |
| 容量少了一个 | 满判断错误 | ring buffer 实际容量 = capacity - 1（留一个空位区分满/空）|

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| aligned 属性 | [6.7.1 地址对齐 aligned](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.7-aligned/6.7.1-地址对齐-aligned.md) |
| __attribute__ 基础 | [6.6.1 __attribute__](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) |

---

## 交付物 4：编译期工具宏

### 做什么

实现内核常用的编译期检查宏：`BUILD_BUG_ON`、`ARRAY_SIZE`、`__same_type`。

### 代码骨架

```c
// src/compile_macros.h
#ifndef COMPILE_MACROS_H
#define COMPILE_MACROS_H

// 编译期断言：条件为真则编译失败
// 原理：变长数组不能有负大小
#define BUILD_BUG_ON(cond) \
    ((void)sizeof(char[1 - 2 * !!(cond)]))

// C11 版本（更清晰）
#define BUILD_BUG_ON_C11(cond) \
    _Static_assert(!(cond), "BUILD_BUG_ON: " #cond)

// 安全的数组大小：同时做类型检查
// __builtin_types_compatible_p 确保传的是数组不是指针
#define ARRAY_SIZE(arr) ({                           \
    BUILD_BUG_ON(!__builtin_types_compatible_p(      \
        typeof(arr), typeof(&(arr)[0])));            \
    sizeof(arr) / sizeof((arr)[0]);                   \
})

// 类型相同检查
#define __same_type(a, b) \
    __builtin_types_compatible_p(typeof(a), typeof(b))

#endif
```

### 分步实现

1. **写 `BUILD_BUG_ON`**：`!!(cond)` 把任意值变成 0/1，`1 - 2*0 = 1`（正常），`1 - 2*1 = -1`（负数组大小 = 编译错误）
2. **写 `ARRAY_SIZE`**：先不加类型检查，直接 `sizeof(arr)/sizeof(arr[0])`；然后加上 `__builtin_types_compatible_p` 检查——传指针会编译报错
3. **测试**：`BUILD_BUG_ON(sizeof(int) == 4)` 在 32 位 int 上会编译失败（正确！）；`int arr[10]; ARRAY_SIZE(arr)` 返回 10；`int *p; ARRAY_SIZE(p)` 编译失败

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| __builtin_ 内建函数 | [6.11.2 常用内建函数](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.11-builtin/6.11.2-常用的内建函数.md) |
| __builtin_constant_p | [6.11.4 __builtin_constant_p](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.11-builtin/6.11.4-内建函数-__builtin_constant_p-n.md) |
| likely/unlikely | [6.11.6 likely 和 unlikely](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.11-builtin/6.11.6-Linux内核中的likely和unlikely.md) |

---

## 交付物 5：函数指针 vtable 模式

### 做什么

用函数指针表实现多态 + `__attribute__((weak))` 做可覆盖的默认实现。

### 代码骨架

```c
// src/vtable.h
#ifndef VTABLE_H
#define VTABLE_H

#include <stddef.h>
#include <stdio.h>

// 操作接口（vtable）
struct storage_ops {
    int  (*open)(void *ctx, const char *path);
    int  (*read)(void *ctx, char *buf, size_t len);
    void (*close)(void *ctx);
};

// 文件后端
struct file_backend {
    FILE *fp;
    struct storage_ops ops;
};

int file_open(void *ctx, const char *path) {
    struct file_backend *fb = ctx;
    fb->fp = fopen(path, "r");
    return fb->fp ? 0 : -1;
}

int file_read(void *ctx, char *buf, size_t len) {
    struct file_backend *fb = ctx;
    return fread(buf, 1, len, fb->fp);
}

void file_close(void *ctx) {
    struct file_backend *fb = ctx;
    if (fb->fp) fclose(fb->fp);
}

// 内存后端
struct mem_backend {
    char *data;
    size_t pos, size;
    struct storage_ops ops;
};

// weak 默认实现：可以被同名强符号覆盖
int __attribute__((weak)) default_open(void *ctx, const char *path) {
    (void)ctx; (void)path;
    return -1;  // 默认不支持
}

#endif
```

### 分步实现

1. **定义 `struct storage_ops`**：3 个函数指针（open/read/close）
2. **写 file_backend**：用 `fopen/fread/fclose` 实现接口
3. **写 mem_backend**：用内存 buffer 实现接口
4. **写一个 `process(ops, ctx)` 函数**：只通过 ops 调用，不关心后端——这就是多态
5. **加 weak 符号**：定义 `default_open` 为 weak，在另一个文件里写同名强符号覆盖，验证覆盖生效

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 函数签名不匹配 | 运行时崩溃 | 函数指针类型必须完全一致（参数、返回值） |
| weak 符号没链接 | 覆盖不生效 | weak 和强符号必须在同一链接范围 |
| 忘了设 ops 函数指针 | 段错误调用 NULL | 初始化时必须逐个赋值 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 函数指针/转移表 | [13.3.2 转移表](../../01-c-language/02-Pointers-on-C/ch13-advanced-pointer-topics/13.3-function-pointers/13.3.2-转移表.md) |
| weak 符号 | [6.9 weak](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.9-weak/6.9-属性声明-weak.md) |

---

## 交付物 6：结构体布局控制

### 做什么

实验 `__attribute__((packed))` / `aligned` / `section` / flexible array member，理解编译器如何布局结构体。

### 代码骨架

```c
// src/layout.c
#include <stdio.h>
#include <stddef.h>

// 默认对齐：编译器会在 char 后插 3 字节 padding
struct normal {
    char  a;    // offset 0
    int   b;    // offset 4 (padding 1-3)
    char  c;    // offset 8
};  // sizeof = 12 (padding 9-11)

// packed：去掉所有 padding
struct __attribute__((packed)) packed_s {
    char  a;    // offset 0
    int   b;    // offset 1
    char  c;    // offset 5
};  // sizeof = 6

// aligned 强制对齐
struct __attribute__((aligned(64))) cache_line {
    int data[8];
};  // sizeof = 64

// flexible array member（C99）
struct flexbuf {
    size_t len;
    char   data[];   // 不占 sizeof，malloc 时多分配
};

// 自定义段
struct init_entry {
    const char *name;
    void (*fn)(void);
};

// 放到自定义段里（链接脚本可以遍历）
static struct init_entry __attribute__((section(".init_table"))) entry1 = {
    "first", NULL
};

int main(void) {
    printf("normal:  sizeof=%zu, b offset=%zu\n",
           sizeof(struct normal), offsetof(struct normal, b));
    printf("packed:  sizeof=%zu, b offset=%zu\n",
           sizeof(struct packed_s), offsetof(struct packed_s, b));
    printf("aligned: sizeof=%zu\n", sizeof(struct cache_line));

    // flexible array
    struct flexbuf *fb = malloc(sizeof(struct flexbuf) + 100);
    fb->len = 100;
    fb->data[0] = 'X';  // 直接用
    free(fb);
    return 0;
}
```

### 分步实现

1. **写上面的代码，编译运行**：看 sizeof 和 offset 的实际值
2. **用 `pahole` 分析**（如果有）：`pahole -C normal layout.o` 看每个成员的偏移和 padding
3. **packed 实验**：用 `__attribute__((packed))`，注意它会让访问未对齐的 `b` 变慢（某些架构直接 SIGBUS）
4. **flexible array**：`malloc(sizeof(struct flexbuf) + n)`，对比老式 `struct { size_t len; char data[0]; }`（GCC 扩展，C99 用 `[]`）

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| packed/aligned | [6.7.4 packed](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.7-aligned/6.7.4-属性声明-packed.md) |
| 内核中的 aligned+packed | [6.7.5 内核中的声明](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.7-aligned/6.7.5-内核中的aligned-packed声明.md) |
| section 属性 | [6.6.2 section](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.6-section/6.6.2-属性声明-section.md) |

---

## 交付物 7：X-Macro 代码生成

### 做什么

用 X-Macro 技术一次定义，自动生成 enum + 字符串数组 + 打印函数。

### 代码骨架

```c
// src/xmacro.c
#include <stdio.h>

// ===== 一次定义 =====
// 每行 X(名字, 值, 字符串)
#define ERROR_TABLE \
    X(ERR_NONE,   0,  "success") \
    X(ERR_NOMEM,  1,  "out of memory") \
    X(ERR_IO,     2,  "I/O error") \
    X(ERR_TIMEOUT, 3, "timeout") \
    X(ERR_INVALID, 4, "invalid argument")

// ===== 生成 enum =====
typedef enum {
#define X(name, val, str) name = val,
    ERROR_TABLE
#undef X
} error_code;

// ===== 生成字符串数组 =====
static const char *error_strings[] = {
#define X(name, val, str) [name] = str,
    ERROR_TABLE
#undef X
};

// ===== 生成打印函数 =====
void print_error(error_code code) {
    if (code >= 0 && code < (int)(sizeof(error_strings)/sizeof(error_strings[0]))
        && error_strings[code])
        printf("Error %d: %s\n", code, error_strings[code]);
    else
        printf("Unknown error: %d\n", code);
}

int main(void) {
    print_error(ERR_NOMEM);   // Error 1: out of memory
    print_error(ERR_TIMEOUT); // Error 3: timeout
    return 0;
}
```

### 分步实现

1. **写 `ERROR_TABLE` 宏**：每行一个 `X(name, val, str)`
2. **生成 enum**：`#define X` 展开成 `name = val,`，`#undef X` 清理
3. **生成字符串数组**：用指定初始化 `[name] = str`
4. **加一个新错误**：只在 `ERROR_TABLE` 里加一行 `X(ERR_NEW, 5, "new error")`，enum/字符串/打印函数全自动更新——这就是 X-Macro 的威力

### 为什么重要

内核 `errno.h`、`syscall.h`、driver 的 `ioctl` 定义都用了类似技术。加一个值只需改一处，不会漏。

---

## 交付物 8：（选做）内嵌汇编

### 做什么

用 `asm volatile` 实现内存屏障和 CPU 时间戳读取。

### 代码骨架

```c
// src/inline_asm.h
#ifndef INLINE_ASM_H
#define INLINE_ASM_H

#include <stdint.h>

// x86 内存屏障
static inline void memory_barrier_x86(void) {
    asm volatile("mfence" ::: "memory");
}

// x86 读 TSC（时间戳计数器）
static inline uint64_t rdtsc(void) {
    uint32_t hi, lo;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

// AArch64 内存屏障
static inline void memory_barrier_arm(void) {
    asm volatile("dmb ish" ::: "memory");
}

// AArch64 读虚拟计数器
static inline uint64_t cntvct(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val) :: "memory");
    return val;
}

#endif
```

### 分步实现

1. **写 `rdtsc`**（x86）或 `cntvct`（ARM）：读 CPU 时间戳计数器
2. **写屏障**：`mfence`（x86）/ `dmb`（ARM），理解 `"memory"` clobber 防编译器重排
3. **测试**：`uint64_t t1 = rdtsc(); do_something(); uint64_t t2 = rdtsc();` 测量周期数
4. **对比**：跟 `clock_gettime(CLOCK_MONOTONIC)` 对比精度

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| __attribute__ 基础 | [6.6.1 __attribute__](../../01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) |

---

## 建议实现顺序

```
交付物 1 (container_of)  ← 基础，后面都依赖
  ↓
交付物 2 (侵入式链表)     ← 用 container_of
  ↓
交付物 4 (编译期宏)       ← 独立，快
  ↓
交付物 3 (ring buffer)    ← 用 aligned + atomic
  ↓
交付物 6 (结构体布局)     ← 独立实验
  ↓
交付物 7 (X-Macro)        ← 独立，快
  ↓
交付物 5 (vtable + weak)  ← 综合
  ↓
交付物 8 (内嵌汇编)       ← 选做
```

每个交付物 30 分钟-2 小时。全部做完约 1-2 天。做完后 P4 内核模块需要的 C 技能全部就位。

## 环境

- WSL Ubuntu 24.04（gcc 13.3 + make）
- 编译：`gcc -std=gnu11 -Wall -Wextra -g -O2`
- 验证对齐：`pahole` / `__alignof__` / `offsetof`
- Makefile 示例：

```makefile
CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -g -O2
TESTS = test_container_of test_list test_ringbuf test_macros test_layout test_xmacro

all: $(TESTS)

test_%: tests/%.c src/*.h
	$(CC) $(CFLAGS) -Isrc $< -o $@

clean:
	rm -f $(TESTS)
```

## 目录约定

```
P2.5-c-toolkit/
  README.md     ← 本指南
  src/          ← 头文件（container_of.h, list.h, ringbuf.h, ...）
  tests/        ← 每个交付物一个测试文件
  notes/        ← 你的踩坑（自己写，不是 AI 代写）
```

## 状态

✅ `part-a-toolkit`：`make test`（container_of / list / ringbuf / ARRAY_SIZE / likely）。布局实验：`make run`。

← [projects 总览](../README.md) · [01-c-language](../../01-c-language/)
