# 第 5 章 C++ 内存模型和原子操作

**The C++ Memory Model and Operations on Atomic Types**

## 本章讲什么

这是全书最难也最核心的一章。讲 C++ 的内存模型（happens-before、synchronizes-with 关系）、`std::atomic` 的各种操作、六种内存序的语义与代价、以及如何用原子操作构建无锁数据结构。理解这一章才能真正写出正确的无锁代码。

## 要点

### 内存模型基础

C++ 内存模型定义了**多线程下操作如何对彼此可见**。核心概念：

| 概念 | 含义 |
|------|------|
| happens-before | A happens-before B：A 的所有内存效果对 B 可见 |
| synchronizes-with | A 的释放操作 synchronizes-with B 的获取操作 |
| sequenced-before | 同一线程内语句的先后顺序 |
| 修改顺序 | 所有线程对同一原子变量的写入达成一致的全序 |

### 六种内存序

```cpp
enum memory_order {
    relaxed,           // 无同步，仅原子性
    consume,           // 数据依赖（实践中几乎等同 acquire，已弃用倾向）
    acquire,           // 读：之后的读写不能重排到之前
    release,           // 写：之前的读写不能重排到之后
    acq_rel,           // 读写都有：RMW 操作用
    seq_cst            // 全局总序，最强，默认
};
```

| 内存序 | 代价 | 典型用法 |
|--------|------|----------|
| `relaxed` | 最低 | 计数器、无依赖的状态标志 |
| `acquire`/`release` | 中 | 配对使用，构建 happens-before |
| `seq_cst` | 最高 | 默认，简单但可能成为瓶颈 |

**关键直觉**：
- `release` 写 + `acquire` 读配对：写线程在 release 前的所有写，对读到该值的读线程可见（建立 happens-before）。
- `seq_cst` 在 acquire/release 基础上额外保证**全局总序**——所有线程看到的操作顺序一致。
- `relaxed` 只保证单个原子变量本身的原子性，不提供任何跨变量同步。

### `std::atomic` 操作

```cpp
std::atomic<int> x{0};

x.store(1, std::memory_order_release);        // 写
int v = x.load(std::memory_order_acquire);    // 读
x.fetch_add(1, std::memory_order_relaxed);    // RMW（读-改-写）
bool ok = x.compare_exchange_strong(expected, desired);  // CAS

x.exchange(5);    // 原子交换
x.is_lock_free(); // 是否真正无锁（可能用内部 mutex）
```

### CAS（Compare-Exchange）—— 无锁的基石

```cpp
int expected = x.load();
while (!x.compare_exchange_weak(expected, desired)) {
    // 失败时 expected 被更新为当前值，重试
}
```

- `compare_exchange_strong`：失败不虚假失败，适合循环内只需一次尝试的场景。
- `compare_exchange_weak`：可能**虚假失败**（值实际等于 expected 却返回 false），适合放在 while 循环里（性能更好，某些平台少一条指令）。
- CAS 是 ABA 问题的高发地：值从 A→B→A，CAS 以为没变过。解决：加版本号（tagged pointer）或 hazard pointer / epoch 回收。

### 原子标志的最简同步

```cpp
std::atomic<bool> ready{false};
// 线程 A
data = 42;
ready.store(true, std::memory_order_release);
// 线程 B
while (!ready.load(std::memory_order_acquire));
assert(data == 42);   // 一定成立：release-acquire 建立了 happens-before
```

### `volatile` ≠ `atomic`

`volatile` 只防止编译器优化（不缓存到寄存器），**不保证**：
- 原子性（`volatile int64_t` 在 32 位机上可能撕裂）
- 可见性（一个线程的写对另一个线程不可见）
- 内存序（不阻止重排）

`std::atomic` 三者都保证。多线程共享变量**必须用 atomic，不要用 volatile**。

## HFT 关联

- **relaxed 用于统计计数**：吞吐量计数器用 `relaxed` 的 `fetch_add`，无同步开销。
- **acquire/release 配对用于序列号同步**：生产者写数据 + `release` 存序列号；消费者 `acquire` 读序列号后再读数据——SPSC 无锁队列的经典模式。
- **seq_cst 是热路径性能杀手**：它需要 CPU 全局内存屏障（x86 上是 `mfence`/`lock` 前缀），比 relaxed 慢数倍。热路径尽量用 acquire/release。
- **CAS 自旋 vs mutex**：临界区极短（几条指令）且竞争不激烈时，CAS 自旋比 mutex 快（避免上下文切换）。但高竞争下 CAS 自旋会烧 CPU，反而更慢。
- **cache 行伪共享**：多个原子变量落在同一 cache 行（64B）会导致 ping-pong。用 `alignas(64)` 或 padding 隔离热变量。
- **x86 的 TSO 优势**：x86 是 TSO（Total Store Order），acquire/load 和 release/store 几乎免费（只有 seq_cst 的 store 需要 `mfence`）。ARM 是弱内存序，acquire/release 也有显式屏障代价——跨平台无锁代码要测 ARM。

## 自测题

1. happens-before 和 synchronizes-with 的关系是什么？release-acquire 如何建立 happens-before？
2. 六种内存序中，为什么 `seq_cst` 是默认但热路径要换成 acquire/release？代价差在哪？
3. `compare_exchange_weak` 和 `strong` 的区别是什么？为什么 weak 适合放在循环里？
4. ABA 问题是什么？在无锁队列中如何发生？怎么解决？
5. 为什么多线程下 `volatile` 不能替代 `atomic`？x86 的 TSO 对无锁编程有什么好处？
