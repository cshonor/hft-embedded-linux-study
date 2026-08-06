# Item 40：std::atomic 用于并发，volatile 用于特殊内存——别混用

> 第 7 章 · Item 40 · 上一节：[Item 39 单次事件](item39-one-shot-events.md)

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

### volatile 在 C++ 里不是线程同步工具

```cpp
volatile bool ready = false;
// 线程 A: ready = true;
// 线程 B: while (!ready) {}  // 可能永远循环！
```

`volatile` 只防编译器把读写优化掉（如硬件寄存器、mmap 内存），**不保证**：①跨核可见性 ②原子性 ③阻止重排。多线程共享变量必须用 `std::atomic` 或 mutex。

---

## 新手要点（和 C 的区别）

- **C 程序员常误用 volatile 做线程同步**：在 C 里 `volatile` 也**不是**线程同步工具——但很多嵌入式代码错误地用它。C++ 的 `std::atomic` 才是正确答案。
- **volatile 的正确用途**：内存映射 I/O（MMIO）、硬件寄存器、信号处理函数里的 `sig_atomic_t`。
- **规则**：多线程共享变量 → `std::atomic`；硬件寄存器/mmap → `volatile`。两者不可互换。

---

## HFT 关联

- **行情计数器**：`std::atomic<uint64_t>` 保证跨核可见性 + 原子性。
- **DPDK 寄存器映射**：`volatile` 只用于 mmap 的硬件寄存器映射（PMD 配置寄存器）。

---

## 自测题

1. `std::atomic` 和 `volatile` 的本质区别是什么？
2. 为什么 `volatile` 不能用于线程同步？它不保证什么？
3. `volatile` 的正确用途是什么？
4. 为什么很多 C 嵌入式代码用 volatile 做线程同步是错的？

---

## 参考与延伸

- 下一章：[第 8 章 微调](../ch08-tweaks/README.md)
- 回到：[第 7 章](README.md)
