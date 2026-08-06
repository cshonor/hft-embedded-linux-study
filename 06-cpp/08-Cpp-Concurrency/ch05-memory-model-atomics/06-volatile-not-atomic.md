# 5.6 volatile ≠ atomic

> 第 5 章 · 上一节：[5.5 原子标志同步](05-atomic-flag.md) · 下一章：[第 6 章 基于锁的容器](../ch06-lock-based-containers/README.md)

## 这节讲什么

`volatile` 只防编译器优化，**不保证**原子性、可见性、内存序。多线程共享变量必须用 `std::atomic`。

---

## 本质区别

`volatile` 只防止编译器把读写缓存到寄存器（如硬件寄存器、mmap 内存），**不保证**：
- 原子性（`volatile int64_t` 在 32 位机上可能撕裂）
- 可见性（一个线程的写对另一个线程不可见）
- 内存序（不阻止重排）

`std::atomic` 三者都保证。

```cpp
volatile bool ready = false;
// 线程 A: ready = true;
// 线程 B: while (!ready) {}  // 可能永远循环！
```

---

## 新手要点（和 C 的区别）

- **C 程序员常误用 volatile 做线程同步**：在 C 里 `volatile` 也**不是**线程同步工具——但很多嵌入式代码错误地用它。
- **volatile 的正确用途**：内存映射 I/O（MMIO）、硬件寄存器、信号处理函数里的 `sig_atomic_t`。
- **规则**：多线程共享变量 → `std::atomic`；硬件寄存器/mmap → `volatile`。两者不可互换。

---

## HFT 关联

- **`atomic` 替代 `volatile`**：行情计数器、序号用 `std::atomic<uint64_t>` 保证跨核可见性 + 原子性。
- **`volatile` 只用于 mmap**：DPDK PMD 配置寄存器映射用 `volatile`，其他场景一律 `atomic`。

---

## 自测题

1. 为什么 `volatile` 不能用于线程同步？它不保证什么？
2. `volatile` 的正确用途是什么？
3. 为什么很多 C 嵌入式代码用 volatile 做线程同步是错的？

---

## 参考与延伸

- 下一章：[第 6 章 基于锁的容器](../ch06-lock-based-containers/README.md)
- 回到：[第 5 章](README.md)
