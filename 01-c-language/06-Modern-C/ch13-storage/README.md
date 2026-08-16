# Ch13 · Storage（存储）

> **Level 2 · 相知** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

C 的四种存储期（storage duration）、`malloc` 家族与 `realloc` 陷阱、初始化规则全表、
机器模型抽象。**HFT 热路径不用 `malloc`**——启动时一次性分配 + 自管理内存池，
理解四种存储期是设计内存池和理解 `_Thread_local` 的基础。

## 一、四种存储期（核心）

C 的每个对象都有"存储期"——决定它的生命周期（何时创建、何时销毁）。

| 存储期 | 关键字 | 生命周期 | 初始值 | 典型用途 |
|--------|--------|----------|--------|----------|
| **自动** (automatic) | 默认（局部变量） | 块进入→退出 | 未初始化=垃圾值 | 临时变量、函数参数 |
| **静态** (static) | `static` / 全局 | 程序开始→结束 | 零初始化 | 全局配置、计数器 |
| **线程** (thread) | `_Thread_local`（C11） | 线程开始→结束 | 零初始化 | 每 lcore 独立数据 |
| **分配** (allocated) | `malloc`/`calloc` | malloc→free | 未初始化（malloc）/ 零（calloc） | 动态数据结构 |

### 自动存储期（栈）

```c
void process(void) {
    int x = 42;           // 自动存储期：进入函数时创建，退出时销毁
    char buf[1024];       // 栈上分配，退出即回收
    // ...
}   // x, buf 在这里销毁
```

| 要点 | 说明 |
|------|------|
| 分配在栈上 | 速度极快（移动栈指针） |
| 生命周期 = 块作用域 | 退出块后内存回收，不可再访问 |
| 未初始化 = 垃圾值 | 局部变量不自动清零 |
| 不要返回栈指针 | `int *f() { int x; return &x; }` → 悬垂指针 |

### 静态存储期（全局/static）

```c
int global_counter = 0;           // 静态存储期：程序开始→结束
static int module_state = 0;      // 静态存储期 + 文件内可见

void init(void) {
    static int initialized = 0;   // 静态存储期 + 块作用域
    if (!initialized) {
        // 只执行一次
        initialized = 1;
    }
}
```

| 要点 | 说明 |
|------|------|
| 零初始化 | 全局/static 变量自动清零（BSS 段） |
| 生命周期 = 程序 | 从 `main` 之前到程序退出 |
| `static` 限定可见性 | 文件级 `static` = 文件内可见；块级 `static` = 块内可见但生命周期是程序级 |
| 线程不安全 | 多线程访问需同步（除非只读） |

> **HFT 注意**：全局可变状态是多线程 bug 源。HFT 架构倾向"启动时初始化全局只读配置，运行时只读"。

### 线程存储期（`_Thread_local`，C11）

```c
/* C11: _Thread_local / C23: thread_local */
_Thread_local unsigned lcore_id;      // 每个线程独立一份
_Thread_local struct stats per_core_stats;  // 每核统计

/* C23：thread_local 是关键字（同 _Thread_local） */
thread_local int counter = 0;
```

| 要点 | 说明 |
|------|------|
| 每线程独立 | 每个线程有自己的副本，互不干扰 |
| 零初始化 | 像静态存储期一样自动清零 |
| 生命周期 = 线程 | 线程开始创建，线程退出销毁 |
| HFT 用途 | 每 lcore 的统计计数器、线程局部缓冲区 |

```c
/* DPDK 模型：每 lcore 一线程，用 thread_local 存每核数据 */
static _Thread_local unsigned lcore_id;
static _Thread_local struct {
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t drops;
} lcore_stats;

int worker_thread(void *arg) {
    lcore_id = rte_lcore_id();   // 设置当前线程的 lcore_id
    while (running) {
        lcore_stats.rx_pkts += rte_eth_rx_burst(...);
        // ...
    }
}
```

> **DPDK 对应**：内核用 `DEFINE_PER_CPU(type, name)`，用户态 C11 用 `_Thread_local`。
> 两者本质相同：每 CPU/线程独立数据，避免共享缓存行。

### 分配存储期（堆）

```c
void *buf = malloc(1024);      // 分配存储期：malloc→free
// ...
free(buf);                     // 释放后不能再访问
buf = NULL;                    // 好习惯：置空防悬垂
```

详见下方 `malloc` 家族。

## 二、`malloc` 家族

### 四个分配函数

| 函数 | 行为 | 初始值 |
|------|------|--------|
| `malloc(size)` | 分配 size 字节 | 未初始化（垃圾值） |
| `calloc(n, size)` | 分配 n×size 字节 | 零初始化 |
| `realloc(ptr, new_size)` | 调整大小 | 旧数据保留，新增部分未初始化 |
| `aligned_alloc(alignment, size)` | 对齐分配（C11） | 未初始化 |

```c
/* malloc：不初始化 */
int *arr = malloc(100 * sizeof(int));   // 内容不确定

/* calloc：零初始化（更安全） */
int *arr2 = calloc(100, sizeof(int));   // 全零

/* aligned_alloc：指定对齐（C11） */
void *buf = aligned_alloc(64, 4096);    // 64 字节对齐，4096 字节
// 注意：size 必须是 alignment 的倍数

/* free：释放 */
free(arr);
free(arr2);
free(buf);
```

> **HFT 建议**：用 `calloc` 替代 `malloc`（零初始化更安全，现代 OS 的 zero-page 机制使 calloc 不比 malloc 慢多少）。`aligned_alloc` 用于需要特定对齐的场景（如 DMA 缓冲区）。

### `realloc` 陷阱

```c
int *arr = malloc(10 * sizeof(int));

/* ❌ 错误写法：realloc 失败时返回 NULL，原指针泄漏 */
arr = realloc(arr, 20 * sizeof(int));   // 失败时 arr = NULL，原内存泄漏！

/* ✅ 正确写法：用临时变量 */
int *tmp = realloc(arr, 20 * sizeof(int));
if (!tmp) {
    free(arr);     // realloc 失败，原内存仍有效，需手动释放
    return -1;     // 或保持 arr 不变继续使用
}
arr = tmp;         // 成功，更新指针
```

| realloc 行为 | 说明 |
|--------------|------|
| 原地扩展 | 如果后面有足够空间，原地扩大 |
| 搬迁到新位置 | 如果原地不够，分配新块，复制旧数据，释放旧块 |
| 返回新指针 | 搬迁后指针地址变化，必须用返回值 |
| 失败返回 NULL | 原内存不受影响，仍需 `free` |
| `realloc(NULL, n)` | 等价于 `malloc(n)` |
| `realloc(p, 0)` | C11 前：等价于 `free(p)`；C11+：实现定义 |

> **HFT 立场**：热路径完全不用 `malloc`/`realloc`。启动时预分配所有内存，运行时从内存池分配。

## 三、HFT 内存池模型

### 为什么不用 malloc

| malloc 的问题 | HFT 影响 |
|---------------|----------|
| 不确定延迟 | 分配可能触发 mmap/brk，延迟微秒级波动 |
| 锁竞争 | 多线程 malloc 需要全局锁 |
| 碎片化 | 长时间运行后分配变慢 |
| 不可预测的 page fault | 首次访问分配的页触发缺页中断 |

### 内存池设计

```c
/* 启动时一次性分配大块内存，运行时从中切分 */
struct mempool {
    void    *base;          // 大块内存基地址（malloc/hugepage 一次性分配）
    size_t   obj_size;      // 每个对象大小
    uint32_t capacity;      // 对象总数
    uint32_t free_head;     // 空闲链表头
    uint32_t *free_list;    // 空闲链表（用索引而非指针，cache 友好）
};

void *pool_alloc(struct mempool *p) {
    if (p->free_head == UINT32_MAX) return NULL;  // 耗尽
    uint32_t idx = p->free_head;
    p->free_head = p->free_list[idx];
    return (char *)p->base + idx * p->obj_size;
}

void pool_free(struct mempool *p, void *obj) {
    uint32_t idx = ((char *)obj - (char *)p->base) / p->obj_size;
    p->free_list[idx] = p->free_head;
    p->free_head = idx;
}
```

> **DPDK `rte_mempool`** 就是工业级内存池：启动时从 hugepage 预分配，运行时 O(1) 分配/释放，
> 无锁（每 lcore 独立 cache）、零拷贝。

### 四种存储期在内存池中的角色

| 存储期 | 内存池中的角色 |
|--------|---------------|
| 静态 | 全局配置、内存池描述符（启动后只读） |
| 线程 | 每 lcore 的本地缓存（无锁分配） |
| 分配 | 池中的对象（从预分配的大块切分） |
| 自动 | 函数内临时变量（栈上，最快） |

## 四、初始化规则全表

| 存储期 | 未显式初始化 | 显式初始化 |
|--------|-------------|-----------|
| 自动（局部变量） | **垃圾值**（indeterminate value） | 按初始化器 |
| 静态（全局/static） | **零初始化** | 按初始化器（必须是常量表达式） |
| 线程（`_Thread_local`） | **零初始化** | 按初始化器 |
| 分配 `malloc` | **未初始化** | 手动 memset/calloc |
| 分配 `calloc` | **零初始化** | — |

```c
int global;              // 静态 → 零初始化 → 0
static int s;            // 静态 → 零初始化 → 0

void f(void) {
    int local;           // 自动 → 垃圾值！
    static int s_local;  // 静态 → 零初始化 → 0
    int *p = malloc(sizeof(int));  // 分配 → 未初始化
    int *q = calloc(1, sizeof(int));  // 分配 → 零初始化

    // C23: {} 零初始化
    int arr[10] = {};    // 全零
    struct msg m = {};   // 全成员零
}
```

> **HFT 红线**：局部变量必须初始化！未初始化的局部变量是 HFT 最常见的"偶发 bug"源——
> debug 构建可能碰巧是零，release 构建可能是垃圾值。

## 五、机器模型（digression）

### 寄存器与内存抽象

C 的抽象机器模型：CPU 有寄存器，内存是线性的字节序列。

```
┌──────────┐     ┌─────────────────────────────┐
│  CPU     │     │  Memory (linear address)     │
│ ┌──────┐ │     │ ┌─────────┐                 │
│ │ regs │ │─────│ │ stack   │ ↓ growing       │
│ └──────┘ │     │ ├─────────┤                  │
│          │     │ │ heap    │ ↑ growing       │
│ ┌──────┐ │     │ ├─────────┤                  │
│ │ cache│ │─────│ │ .bss    │ (zero-init globals) │
│ └──────┘ │     │ ├─────────┤                  │
│          │     │ │ .data   │ (init globals)   │
│          │     │ ├─────────┤                  │
│          │     │ │ .text   │ (code)           │
└──────────┘     │ └─────────┘                  │
                 └─────────────────────────────┘
```

| 段 | 存储期 | 内容 |
|----|--------|------|
| `.text` | — | 代码（机器指令） |
| `.data` | 静态 | 已初始化的全局/static 变量 |
| `.bss` | 静态 | 零初始化的全局/static 变量（不占文件空间） |
| 栈 (stack) | 自动 | 局部变量、函数帧 |
| 堆 (heap) | 分配 | malloc 分配的内存 |

> 详见 [CSAPP Ch7](../../06.6-systems-performance/) 和 [K&R 4.3.1 程序内存布局](../01-Primer-K-and-R-C/ch04-functions-and-program-structure/4.3.1-程序内存布局.md)。

### `register` 关键字（C23 废弃）

```c
/* C89：register 是建议编译器放寄存器 */
register int i;     // C23：register 已无语义（保留语法但不做任何事）
```

| 标准 | `register` 语义 |
|------|----------------|
| C89 | 建议编译器放寄存器（不能取地址） |
| C11 | 建议放寄存器（不能取地址） |
| C23 | **无语义**（保留语法，可以取地址） |

> 现代编译器自己做得比人好——`register` 已无意义，不要用。

## HFT / DPDK 关联总结

| 概念 | HFT 应用 |
|------|----------|
| **四种存储期** | 设计内存池、理解 `_Thread_local` 模型 |
| **`_Thread_local`** | 每 lcore 独立数据（统计计数器、本地缓存） |
| **内存池** | 启动时预分配，运行时 O(1) 分配/释放 |
| **`calloc`** | 零初始化分配（比 malloc 安全） |
| **`aligned_alloc`** | DMA 缓冲区对齐分配 |
| **`realloc` 陷阱** | 用临时变量接收返回值 |
| **初始化规则** | 局部变量必须显式初始化 |
| **不用 `malloc` 在热路径** | 延迟不可控 |

## 自测题

<details><summary>1. 四种存储期分别是什么？各自何时创建/销毁？</summary>

① 自动存储期（局部变量）：进入块时创建，退出块时销毁。② 静态存储期（全局/static）：
程序开始时创建，程序结束时销毁。③ 线程存储期（`_Thread_local`）：线程开始时创建，
线程退出时销毁。④ 分配存储期（malloc）：malloc 时创建，free 时销毁。
</details>

<details><summary>2. 为什么 HFT 热路径不用 <code>malloc</code>？怎么替代？</summary>

malloc 的延迟不可预测：可能触发 mmap/brk 系统调用、需要锁竞争、可能 page fault。
HFT 要求微秒级确定性延迟。替代方案：启动时一次性分配大块内存（hugepage），运行时从内存池
O(1) 切分（空闲链表或 slab 分配器），无锁（每 lcore 独立 cache）。DPDK `rte_mempool` 就是工业实现。
</details>

<details><summary>3. <code>arr = realloc(arr, new_size)</code> 有什么问题？正确写法是什么？</summary>

realloc 失败时返回 NULL，但原内存仍有效。`arr = realloc(arr, new_size)` 在失败时把 NULL 赋给 arr，
丢失了原指针 → 内存泄漏。正确写法：`int *tmp = realloc(arr, new_size); if (!tmp) { free(arr); return -1; } arr = tmp;`
</details>

<details><summary>4. <code>_Thread_local</code> 和全局变量有什么区别？</summary>

全局变量是所有线程共享的——一个线程修改，其它线程立刻看到（需同步）。`_Thread_local` 变量是
每个线程独立一份——一个线程修改不影响其它线程的副本，不需要同步。HFT 中每 lcore 的统计计数器
用 `_Thread_local`，避免共享缓存行的伪共享和锁开销。
</details>

<details><summary>5. 为什么局部变量不初始化是 HFT bug 的常见原因？</summary>

自动存储期的局部变量不自动清零——初值是栈上的垃圾值。debug 构建中栈可能碰巧是零，
release 构建中优化后栈内容不同，导致"debug 正常、release 偶发"的 bug。HFT 代码规范应要求
所有局部变量声明时初始化：`int x = 0;` 或 `int *p = NULL;`。
</details>
