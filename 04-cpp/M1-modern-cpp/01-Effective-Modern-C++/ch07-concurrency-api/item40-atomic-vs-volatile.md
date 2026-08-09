# Item 40：std::atomic 用于并发，volatile 用于特殊内存——别混用

> 第 7 章 · Item 40 · 上一节：[Item 39 单次事件](item39-one-shot-events.md)

## 为什么要学这个（先建立直觉）

C 嵌入式程序员常用 `volatile` 做线程间同步——这是**错误的**，但在单核时代"碰巧能用"：

```c
volatile int ready = 0;

// 线程 A
ready = 1;

// 线程 B
while (!ready) { }  // 单核时代"碰巧能用"
// 多核时代：可能永远循环！
// 因为 volatile 不保证跨核可见性
```

C++ 程序员也常犯同样的错误——以为 `volatile` 和 Java 的 `volatile` 一样能做线程同步。**C/C++ 的 `volatile` 和 Java 的 `volatile` 完全不同！**

C++ 的 `std::atomic` 才是正确的线程同步工具：

```cpp
std::atomic<bool> ready{false};

// 线程 A
ready.store(true);  // 原子写 + 内存屏障 → 跨核可见

// 线程 B
while (!ready.load()) { }  // 原子读 → 保证看到 A 的写入
```

---

## 这节讲什么

`std::atomic` 和 `volatile` **完全不同**——这是 C++ 程序员最普遍的误解之一。混用是经典数据竞争来源。

---

## 本质区别

| | `std::atomic` | `volatile` |
|---|---------------|------------|
| 目的 | 多线程数据竞争 | 告诉编译器"别优化此内存访问"（MMIO） |
| 可见性 | 保证（内存屏障） | **不保证**跨线程可见性 |
| 原子性 | 保证 | 不保证 |
| 指令重排 | 阻止相关重排 | **不阻止** |
| 适用场景 | 多线程共享变量 | 硬件寄存器/mmap 内存 |

### volatile 在 C++ 里不是线程同步工具

```cpp
volatile bool ready = false;
// 线程 A: ready = true;
// 线程 B: while (!ready) {}  // 可能永远循环！
```

`volatile` 只防编译器把读写优化掉（如硬件寄存器、mmap 内存），**不保证**：
1. 跨核可见性（CPU 缓存一致性）
2. 原子性（读-改-写操作可能被中断）
3. 指令重排（编译器和 CPU 都可能重排 volatile 变量周围的指令）

### std::atomic 保证什么

```cpp
std::atomic<int> counter{0};

// 原子读-改-写
counter.fetch_add(1);  // 原子递增，不会被打断

// 内存序控制
counter.store(42, std::memory_order_release);  // 之前的写入对其他线程可见
counter.load(std::memory_order_acquire);        // 看到 release 之前的所有写入

// compare-and-swap（无锁编程基础）
int expected = 0;
bool success = counter.compare_exchange_strong(expected, 1);
// 如果 counter == 0，设为 1，返回 true
// 否则 expected = counter 的当前值，返回 false
```

### volatile 的正确用途

```cpp
// 硬件寄存器映射（MMIO）
volatile uint32_t* reg = (volatile uint32_t*)0x40021000;
*reg = 0x01;        // 写硬件寄存器——volatile 防止编译器优化掉
uint32_t val = *reg; // 读硬件寄存器——volatile 防止编译器缓存上次的值

// 信号处理函数
volatile sig_atomic_t flag = 0;
void handler(int) { flag = 1; }  // 信号处理函数中安全
```

多线程共享变量必须用 `std::atomic` 或 mutex。

---

## 常见错误（新手踩坑）

**错误 1：用 volatile 做线程同步**
```cpp
volatile bool stop = false;
// 线程 A: stop = true;
// 线程 B: while (!stop) { work(); }  // 可能永远循环！
```
**修正：** `std::atomic<bool> stop{false};`

**错误 2：用 volatile 保证原子性**
```cpp
volatile int counter = 0;
// 多线程: counter++;  // 不是原子操作！读-改-写三步可能被打断
```
**修正：** `std::atomic<int> counter{0}; counter.fetch_add(1);`

**错误 3：用 atomic 访问硬件寄存器**
```cpp
std::atomic<uint32_t>* reg = ...;
// atomic 可能用锁（如果平台不支持原子操作），对 MMIO 不安全
// volatile 保证每次都真正读写内存，不被优化
```
**修正：** 硬件寄存器用 `volatile`，不用 `atomic`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程同步 | 误用 `volatile`（错误） | `std::atomic`（正确） | C++ 有标准原子操作 |
| 硬件寄存器 | `volatile` | `volatile`（相同） | 防优化 |
| 原子操作 | 平台相关（GCC __sync） | `std::atomic` | 跨平台标准 |
| 内存序 | 不适用 | `memory_order_*` | 精细控制 |

**一句话总结：** C 程序员记住——多线程共享变量 → `std::atomic`；硬件寄存器/mmap → `volatile`。两者不可互换。C 嵌入式代码用 `volatile` 做线程同步是历史遗留错误。

---

## HFT 关联

- **行情计数器**：`std::atomic<uint64_t>` 保证跨核可见性 + 原子性。
- **DPDK 寄存器映射**：`volatile` 只用于 mmap 的硬件寄存器映射（PMD 配置寄存器）。
- **无锁队列**：`std::atomic` 的 `compare_exchange` 是无锁编程的基础——HFT 用无锁队列减少锁竞争。
- **停止标志**：`std::atomic<bool> running{true}` 替代 `volatile bool`——保证跨核可见性和指令不重排。

---

## 自测题

1. `std::atomic` 和 `volatile` 的本质区别是什么？
2. 为什么 `volatile` 不能用于线程同步？它不保证什么？
3. `volatile` 的正确用途是什么？
4. 为什么很多 C 嵌入式代码用 volatile 做线程同步是错的？
5. 下面代码有什么问题？
```cpp
volatile int counter = 0;
// thread A: counter++;
// thread B: counter++;
```

---

## 参考与延伸

- 下一章：[第 8 章 微调](../ch08-tweaks/README.md)
- 回到：[第 7 章](README.md)
