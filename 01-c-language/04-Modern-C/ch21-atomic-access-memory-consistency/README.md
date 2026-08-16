# Ch21 · Atomic access and memory consistency（原子访问与内存一致性）

> **Level 3 · 深入** · 策略：**🔴 精读**（全书压轴、DPDK rte_ring 的理论基础）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

数据竞争与 UB、**happens-before 关系**、**五种内存序**（`seq_cst` / `acquire` / `release` /
`acq_rel` / `relaxed`）、原子操作 vs 锁的对比、内存屏障。
**读懂本章才能读懂 DPDK `rte_ring` 无锁队列源码。**

## 一、数据竞争（Data Race）

### 什么是数据竞争

**数据竞争** = 两个线程并发访问同一内存位置，且至少一个是写操作，且没有同步关系。

```c
/* ❌ 数据竞争：两个线程同时写 counter */
int counter = 0;       // 普通 int，不是 _Atomic

// 线程 A            // 线程 B
counter++;            counter++;

/* counter++ 不是原子的：它做三件事
   1. 读 counter
   2. 加 1
   3. 写回 counter

   如果两个线程交错执行：
   A 读 counter (0)
                  B 读 counter (0)
   A 写 counter (1)
                  B 写 counter (1)    ← 结果是 1，不是 2！
*/
```

### 数据竞争 = UB

C11 标准明确规定：**有数据竞争的程序行为是未定义的（UB）**。

| 表现 | 说明 |
|------|------|
| 结果不确定 | 可能丢失更新（上面例子） |
| 编译器优化后更糟 | 编译器可能缓存到寄存器，根本不从内存读 |
| 平台相关 | x86 强内存模型可能"碰巧正确"，ARM/POWER 弱内存模型可能出错 |
| 不可复现 | debug 正确，release 出错；单核正确，多核出错 |

### 解决方案：`_Atomic`

```c
/* ✅ 用 _Atomic 消除数据竞争 */
#include <stdatomic.h>

_Atomic int counter = 0;

// 线程 A                    // 线程 B
atomic_fetch_add(&counter, 1);   atomic_fetch_add(&counter, 1);

/* atomic_fetch_add 是原子的：读-改-写不可分割
   结果一定正确：counter = 2 */
```

| 方案 | 原理 | 适用场景 |
|------|------|----------|
| `_Atomic` + 原子操作 | 硬件级原子指令（LOCK XADD / LDREX+STREX） | 计数器、标志、无锁队列 |
| Mutex | 锁保护临界区 | 复杂数据结构（链表、树） |
| `_Thread_local` | 每线程独立，不共享 | 每 lcore 统计 |

## 二、Happens-before 关系

### 什么是 happens-before

**happens-before** 是 C 内存模型的核心概念：如果 A happens-before B，
那么 A 的效果（写操作）对 B 可见。

```
happens-before 不是"时间上先发生"——它是一个逻辑关系：
  即使 A 在时间上先于 B 执行，如果没有 happens-before 关系，
  B 也可能看不到 A 的写效果（因为 cache、重排等）。
```

### 建立 happens-before 的方式

| 方式 | 说明 | 例子 |
|------|------|------|
| 程序顺序 | 同一线程内，前面的语句 happens-before 后面的 | `a = 1; b = 2;` → a=1 hb b=2 |
| 原子操作同步 | acquire 操作看到 release 操作的写 → 建立 hb | 见下方详解 |
| mutex | unlock happens-before 下一个 lock | `mtx_unlock` hb `mtx_lock` |
| 线程创建/加入 | `thrd_create` 之前的操作 hb 新线程的开始 | 主线程初始化 hb worker 线程读取 |
| 线程加入 | 线程结束 hb `thrd_join` 返回 | worker 的写 hb 主线程 join 后读 |

### 没有 happens-before 的例子

```c
/* ❌ 没有 happens-before，数据竞争 */
int data = 0;
int ready = 0;      // 普通 int，不是 _Atomic

// 线程 A（生产者）        // 线程 B（消费者）
data = 42;                  while (!ready) { }
ready = 1;                  printf("%d\n", data);  // 可能打印 0！

/* 为什么可能打印 0？
   1. 编译器可能重排 ready = 1 到 data = 42 前面（没有依赖关系）
   2. CPU 可能乱序执行（x86 较强，ARM/POWER 会重排）
   3. 消费者的 data 读取可能在 ready 读取之前执行

   data = 42 和 ready = 1 之间没有 happens-before 关系保证
*/
```

```c
/* ✅ 用 _Atomic + acquire/release 建立 happens-before */
_Atomic int data = 0;
_Atomic int ready = 0;

// 线程 A（生产者）              // 线程 B（消费者）
data = 42;                       while (atomic_load(&ready, memory_order_acquire) == 0) {}
atomic_store(&ready, 1,          printf("%d\n", atomic_load(&data, memory_order_relaxed));
    memory_order_release);
                                 /* ✅ 一定打印 42！
                                    release store 与 acquire load 配对，
                                    建立 happens-before：data=42 hb printf(data)
                                 */
```

## 三、五种内存序（核心）

C11 定义了五种内存序，从最强到最弱：

### 概览

| 内存序 | 强度 | 语义 | 性能 | 适用场景 |
|--------|------|------|------|----------|
| `memory_order_seq_cst` | 最强 | 全局顺序一致 | 最慢 | 默认（最安全） |
| `memory_order_acquire` | 中 | 获取：后续读写不重排到此之前 | 中 | 消费者读（加载） |
| `memory_order_release` | 中 | 释放：之前的读写不重排到此之后 | 中 | 生产者写（存储） |
| `memory_order_acq_rel` | 中 | 获取+释放（用于 RMW 操作） | 中 | CAS、fetch_add |
| `memory_order_relaxed` | 最弱 | 只保证原子性，不保证顺序 | 最快 | 计数器、统计 |

### 1. `memory_order_seq_cst`（顺序一致）

```c
/* 默认内存序：所有线程看到一致的内存操作顺序 */
atomic_store(&x, 1, memory_order_seq_cst);   // 等价于 atomic_store(&x, 1)
atomic_load(&y, memory_order_seq_cst);       // 等价于 atomic_load(&y)
```

| 特点 | 说明 |
|------|------|
| 全局顺序 | 所有线程看到所有 seq_cst 操作的全局一致顺序 |
| 最强保证 | 相当于 acquire + release + 全局屏障 |
| 最慢 | 需要 CPU 内存屏障（mfence / dmb） |
| 默认选择 | 不确定时用 seq_cst（最安全） |

### 2. `memory_order_acquire`（获取）

```c
/* acquire 用于 load：保证 acquire 之后的读写不会被重排到 acquire 之前 */
int r = atomic_load(&ready, memory_order_acquire);
/* acquire 之后的读写不会在 acquire 之前执行 */
printf("%d\n", data);   // ✅ 保证看到 release 之前的 data 写入
```

```
acquire load 的屏障效果（单向屏障）：
  之前的读写          之后的读写
  ←───── 不能往下重排 ←──── acquire load
                        ↑ 之后的读不能提到 acquire 前
                        ↑ 之后的写不能提到 acquire 前
  但 acquire 之前的操作可以往后重排（单向）
```

### 3. `memory_order_release`（释放）

```c
/* release 用于 store：保证 release 之前的读写不会被重排到 release 之后 */
data = 42;   // 之前的写
/* release 之前的操作不会在 release 之后执行 */
atomic_store(&ready, 1, memory_order_release);
```

```
release store 的屏障效果（单向屏障）：
  之前的读写
  ─────→ 不能往下重排 ─────→ release store
                            ↑ 之前的读不能推到 release 后
                            ↑ 之前的写不能推到 release 后
  但 release 之后的操作可以往前重排（单向）
```

### 4. acquire + release 配对（最重要的模式）

```
生产者（线程 A）：              消费者（线程 B）：

data = 42;                      ┌─ acquire load ─┐
buf[i] = item;                  │  while (!ready) │  ← 看到 ready=1 后
... (其它写)                    │     load(acq)   │
release store ──→ ready = 1 ──→ └─────────────────┘
                                data 和 buf[i] 的写入都可见！

happens-before 链：
  data=42 ──hb──> release store(ready=1)
                         │
                         ↓ (acquire 看到 release)
                  acquire load(ready) ──hb──> read(data) ✅
```

```c
/* 完整示例：生产者-消费者用 acquire/release */
#include <stdatomic.h>
#include <threads.h>
#include <stdio.h>

_Atomic int data = 0;
_Atomic int ready = 0;

int producer(void *arg) {
    (void)arg;
    atomic_store(&data, 42, memory_order_relaxed);    // 普通写
    atomic_store(&ready, 1, memory_order_release);     // release：之前的写对 acquire 可见
    return 0;
}

int consumer(void *arg) {
    (void)arg;
    while (atomic_load(&ready, memory_order_acquire) == 0)
        ;   // 自旋等待
    printf("data = %d\n", atomic_load(&data, memory_order_relaxed));  // ✅ 一定打印 42
    return 0;
}
```

> **这是 DPDK rte_ring 的核心模式**：生产者写数据后用 release 发布，消费者用 acquire 获取。
> 详见下方 rte_ring 分析。

### 5. `memory_order_acq_rel`（获取-释放）

```c
/* acq_rel 用于 RMW 操作（Read-Modify-Write）：同时是 acquire 和 release */
/* 用于 CAS（compare_exchange）和 fetch_add 等 */
int old = atomic_fetch_add(&counter, 1, memory_order_acq_rel);
/* 既保证看到之前 release 的写（acquire），又让之后的 acquire 看到这次写（release） */
```

### 6. `memory_order_relaxed`（宽松）

```c
/* relaxed：只保证操作本身原子，不保证任何顺序 */
atomic_fetch_add(&counter, 1, memory_order_relaxed);
/* counter 一定正确递增，但与其它变量的读写没有顺序保证 */
```

| 适用场景 | 说明 |
|----------|------|
| 计数器 | 只关心总数正确，不关心与其它操作的顺序 |
| 统计 | rx_pkts、tx_pkts 等性能计数器 |
| 引用计数 | `atomic_fetch_add(&refcount, 1, memory_order_relaxed)` |

> **HFT 场景**：每 lcore 的统计计数器用 `relaxed`——只关心数字正确，不需要与其它操作建立顺序关系。
> 但引用计数释放（`fetch_sub` 到 0 时释放）需要 `acq_rel`——必须看到之前的所有写才能安全释放。

### 内存序选择决策表

| 场景 | load | store | RMW |
|------|------|-------|-----|
| 不确定（最安全） | seq_cst | seq_cst | seq_cst |
| 生产者发布数据 | — | release | — |
| 消费者获取数据 | acquire | — | — |
| CAS 更新共享数据 | — | — | acq_rel |
| 纯计数器 | relaxed | relaxed | relaxed |
| 引用计数递增 | — | — | relaxed |
| 引用计数递减（可能释放） | — | — | acq_rel |

## 四、原子操作 API

### 基本操作

```c
#include <stdatomic.h>

_Atomic int x = 0;

/* 加载/存储 */
int val = atomic_load(&x);                          // 默认 seq_cst
int val2 = atomic_load(&x, memory_order_acquire);   // 指定内存序
atomic_store(&x, 42);                               // 默认 seq_cst
atomic_store(&x, 42, memory_order_release);         // 指定内存序

/* 交换 */
int old = atomic_exchange(&x, 99);                  // 原子地写入新值，返回旧值

/* 比较-交换（CAS） */
int expected = 42;
bool success = atomic_compare_exchange_strong(&x, &expected, 99);
// 如果 x == expected：写入 99，返回 true
// 如果 x != expected：把 x 的当前值写入 expected，返回 false
```

### RMW（Read-Modify-Write）操作

```c
/* fetch_add：原子地加，返回旧值 */
int old = atomic_fetch_add(&x, 1);    // x 变为 x+1，返回 x 的旧值

/* fetch_sub / fetch_or / fetch_and / fetch_xor / fetch_max / fetch_min */
atomic_fetch_or(&flags, 0x01);        // 原子地置位
atomic_fetch_and(&flags, ~0x01);      // 原子地清位
```

| 操作 | 返回值 | 说明 |
|------|--------|------|
| `atomic_load` | 当前值 | 原子读 |
| `atomic_store` | void | 原子写 |
| `atomic_exchange` | 旧值 | 原子替换 |
| `atomic_compare_exchange_strong` | bool | CAS：相等则替换 |
| `atomic_compare_exchange_weak` | bool | CAS（可能虚假失败，用于循环） |
| `atomic_fetch_add` | 旧值 | 原子加 |
| `atomic_fetch_sub` | 旧值 | 原子减 |
| `atomic_fetch_or` | 旧值 | 原子或 |
| `atomic_fetch_and` | 旧值 | 原子与 |

### CAS 循环（无锁编程核心模式）

```c
/* CAS 循环：乐观更新——读、计算、CAS，失败则重试 */
void atomic_increment_to(_Atomic int *x, int new_val) {
    int old_val = atomic_load(x, memory_order_relaxed);
    while (old_val < new_val) {
        if (atomic_compare_exchange_weak(x, &old_val, new_val,
                memory_order_acq_rel, memory_order_relaxed))
            break;   // CAS 成功
        // CAS 失败：old_val 已被更新为当前值，重试
    }
}
```

| CAS 类型 | 说明 |
|----------|------|
| `compare_exchange_strong` | 一定成功或失败（不会虚假失败） |
| `compare_exchange_weak` | 可能虚假失败（值相等也返回 false），但在循环中更高效 |

> CAS 是无锁数据结构的基础：DPDK `rte_ring` 的入队/出队就是 CAS 循环（或 head/tail 原子推进）。

## 五、DPDK rte_ring 分析

### rte_ring 的内存序使用

```c
/* DPDK rte_ring 的简化模型（SPSC — 单生产者单消费者） */

struct rte_ring {
    _Alignas(64) _Atomic uint32_t head;   // 生产者写（独占缓存行）
    _Alignas(64) _Atomic uint32_t tail;   // 消费者写（独占缓存行）
    uint32_t mask;
    void *slots[];
};

/* 生产者入队 */
int ring_enqueue(struct rte_ring *r, void *item) {
    uint32_t head = atomic_load(&r->head, memory_order_relaxed);   // 自己的 head，relaxed 够
    uint32_t tail = atomic_load(&r->tail, memory_order_acquire);   // 消费者的 tail，需要 acquire
    //                                                ^^^^^^^^
    //  acquire 保证：看到 tail 的最新值后，之前消费者释放的 slot 数据也可见

    if (head - tail >= r->mask + 1)    // 队列满
        return -ENOENT;

    r->slots[head & r->mask] = item;   // 写数据
    atomic_store(&r->head, head + 1, memory_order_release);   // 发布
    //                          ^^^^^^^^
    //  release 保证：slots 写入对消费者可见后，head 更新才可见
    return 0;
}

/* 消费者出队 */
void *ring_dequeue(struct rte_ring *r) {
    uint32_t tail = atomic_load(&r->tail, memory_order_relaxed);   // 自己的 tail
    uint32_t head = atomic_load(&r->head, memory_order_acquire);   // 生产者的 head
    //                                                ^^^^^^^^
    //  acquire 保证：看到 head 的新值后，生产者写入的 slots 数据可见

    if (tail == head)    // 队列空
        return NULL;

    void *item = r->slots[tail & r->mask];   // 读数据
    atomic_store(&r->tail, tail + 1, memory_order_release);   // 发布
    return item;
}
```

### rte_ring 的 happens-before 链

```
生产者：                                消费者：
slots[head] = item;                     ┌─ acquire load(head) ─┐
release store(head+1) ──────────────────→│  看到 head+1         │
                                        └──────────────────────┘
                                         item = slots[tail];   ← ✅ 一定能看到生产者写的 item
                                         release store(tail+1)
```

| 内存序 | 位置 | 为什么 |
|--------|------|--------|
| `relaxed` | 读自己的 head/tail | 单生产者/单消费者，自己的变量不需要同步 |
| `acquire` | 读对方的 head/tail | 必须看到对方 release 之前的所有写（slots 数据） |
| `release` | 写自己的 head/tail | 让对方 acquire 时看到自己的 slots 写入 |

### MPMC（多生产者多消费者）的 CAS

```c
/* 多生产者入队：用 CAS 争抢 head 位置 */
int ring_enqueue_mp(struct rte_ring *r, void *item) {
    uint32_t head;
    do {
        head = atomic_load(&r->head, memory_order_relaxed);
        uint32_t tail = atomic_load(&r->tail, memory_order_acquire);
        if (head - tail >= r->mask + 1)
            return -ENOENT;
        // CAS：尝试把 head 从 head 改为 head+1
    } while (!atomic_compare_exchange_strong(&r->head, &head, head + 1,
             memory_order_acq_rel, memory_order_relaxed));
    //                                     ^^^^^^^^^^^
    //  acq_rel：既 acquire（看到其它生产者的写）又 release（让消费者看到）

    r->slots[head & r->mask] = item;
    /* 多生产者还需要等待前序生产者完成写入（忙等待） */
    return 0;
}
```

## 六、原子操作 vs 锁

| 方面 | `_Atomic` / 无锁 | Mutex |
|------|------------------|-------|
| 延迟 | 纳秒级（CAS 指令） | 微秒级（系统调用 / futex） |
| 阻塞 | 非阻塞（CAS 失败则重试） | 阻塞（等锁时睡眠） |
| 公平性 | 无（可能饥饿） | 有（FIFO） |
| 适用 | 简单操作（计数器、队列） | 复杂数据结构（链表、树） |
| 调试 | 难（内存序 bug 难复现） | 相对容易 |
| HFT 热路径 | ✅ 用无锁 | ❌ 不用锁 |

> **HFT 选择**：热路径用无锁（`_Atomic` + 内存序），初始化/配置用 mutex。
> 无锁不是万能——复杂数据结构用锁更安全更可维护。

## 七、内存屏障

### 为什么需要屏障

CPU 和编译器都会重排内存操作以提高性能。内存屏障阻止重排：

| 屏障类型 | x86 指令 | 效果 |
|---------|---------|------|
| 全屏障 | `mfence` | 之前的读写不重排到之后 |
| 写屏障 | `sfence` | 之前的写不重排到之后的写 |
| 读屏障 | `lfence` | 之前的读不重排到之后的读 |
| C11 等价 | `atomic_thread_fence` | 标准化的屏障 |

```c
#include <stdatomic.h>

/* C11 内存屏障 */
atomic_thread_fence(memory_order_release);   // 之前的读写不重排到之后
atomic_thread_fence(memory_order_acquire);   // 之后的读写不重排到之前
```

### x86 vs ARM 的内存模型差异

| 架构 | 模型 | 重排行为 |
|------|------|----------|
| x86-64 | TSO（Total Store Order） | 较强：只重排 Store-Load |
| ARM | Weak | 较弱：可重排 Load-Load, Load-Store, Store-Store, Store-Load |

```
x86 上"碰巧正确"的代码在 ARM 上可能出错：
  // 线程 A
  data = 42;        // Store
  ready = 1;        // Store
  // x86: 两个 Store 不重排 → ready=1 时 data 一定已写
  // ARM: 两个 Store 可能重排 → ready=1 时 data 可能还没写！
```

> **HFT 注意**：在 x86 上测试正确的无锁代码，部署到 ARM（如 DPDK on ARM 服务器）时可能出错。
> 必须严格使用 C11 原子操作和内存序，不要依赖平台特定的内存模型。

## HFT / DPDK 关联总结

| 概念 | DPDK 应用 |
|------|----------|
| **acquire/release 配对** | rte_ring 的 head/tail 更新 |
| **`relaxed`** | 每 lcore 统计计数器 |
| **CAS 循环** | rte_ring MPMC 入队/出队 |
| **`_Alignas(64)` + `_Atomic`** | 防伪共享 + 原子更新 |
| **acq_rel** | CAS 操作（同时获取和释放） |
| **内存屏障** | `rte_smp_wmb()`/`rte_smp_rmb()`（C11 前）→ `_Atomic`（C11 后） |

### rte_ring 内存序映射

| rte_ring 操作 | C11 内存序 | 说明 |
|---------------|-----------|------|
| 生产者读自己的 head | `relaxed` | 单生产者，不需要同步 |
| 生产者读消费者的 tail | `acquire` | 必须看到消费者释放的 slot |
| 生产者写 head | `release` | 让消费者看到 slots 写入 |
| 消费者读自己的 tail | `relaxed` | 单消费者，不需要同步 |
| 消费者读生产者的 head | `acquire` | 必须看到生产者写入的 slots |
| 消费者写 tail | `release` | 让生产者看到 slot 已释放 |
| MPMC CAS | `acq_rel` | 同时获取和释放 |

## 自测题

<details><summary>1. 为什么 <code>int counter = 0; counter++;</code> 在多线程下不安全？</summary>

`counter++` 不是原子操作——它做三步：读 counter、加 1、写回 counter。两个线程同时执行时，
可能都读到旧值 0，各自加 1 后写回 1，结果丢失了一次更新。C11 标准规定有数据竞争的程序是 UB。
解决：用 `_Atomic int counter` + `atomic_fetch_add(&counter, 1)`，硬件保证读-改-写不可分割。
</details>

<details><summary>2. acquire 和 release 怎么配对建立 happens-before？</summary>

生产者用 `release` 存储（`atomic_store(&flag, 1, memory_order_release)`），消费者用 `acquire`
加载（`atomic_load(&flag, memory_order_acquire)`）。当 acquire 加载看到 release 存储的值时，
happens-before 关系建立：release 之前的所有写操作对 acquire 之后的读操作可见。
这是无锁数据结构传递数据的标准模式——rte_ring 的 head/tail 更新就是这个模式。
</details>

<details><summary>3. 为什么 rte_ring 读自己的 head/tail 用 <code>relaxed</code>，读对方的用 <code>acquire</code>？</summary>

读自己的 head/tail 不需要与其它线程同步——单生产者只有一个线程写 head，读自己的变量用 relaxed
就够了（程序顺序保证）。读对方的 head/tail 需要 acquire——必须看到对方 release 之前的所有写
（即 slots 数据），否则可能读到未初始化的数据。这是 SPSC 环的优化：自己的变量不需要同步开销。
</details>

<details><summary>4. <code>memory_order_relaxed</code> 什么时候安全使用？</summary>

当你只关心操作本身的原子性，不关心与其它操作的顺序时。典型场景：① 统计计数器
（`rx_pkts++`，只关心数字正确，不关心与数据处理的顺序）；② 引用计数递增（只加不减，
不涉及释放）。不安全场景：发布数据（需要 release）、获取数据（需要 acquire）、
CAS 更新（需要 acq_rel）。
</details>

<details><summary>5. x86 上测试正确的无锁代码为什么在 ARM 上可能出错？</summary>

x86 是强内存模型（TSO），大部分重排被硬件禁止——比如 Store-Store 不重排，所以
`data=42; ready=1;` 在 x86 上天然保证 ready=1 时 data 已写。ARM 是弱内存模型，
Store-Store 可能被重排——`data=42; ready=1;` 在 ARM 上 ready 可能先于 data 可见。
必须用 C11 原子操作（`atomic_store(&ready, 1, memory_order_release)`）保证顺序，
不能依赖平台内存模型。
</details>

<details><summary>6. DPDK rte_ring 为什么不用 mutex？</summary>

mutex 有上下文切换开销（竞争时）、不确定延迟（等锁时间取决于其它线程）、不适合 HFT 热路径。
rte_ring 用 `_Atomic` + acquire/release 内存序实现无锁队列：生产者用 release 发布 head，
消费者用 acquire 获取 head——数据通过 happens-before 关系传递，不需要锁。延迟在纳秒级
（CAS 指令），比 mutex 快 1000 倍。
</details>
