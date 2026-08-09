# 5.1 内存模型基础

> 第 5 章 内存模型和原子操作 · 下一节：[5.2 六种内存序](02-memory-orders.md)

## 这节讲什么

C++ 内存模型定义了多线程下操作如何对彼此可见。核心概念：happens-before、synchronizes-with、sequenced-before。

---

## 核心概念

| 概念 | 含义 |
|------|------|
| happens-before | A happens-before B：A 的所有内存效果对 B 可见 |
| synchronizes-with | A 的释放操作 synchronizes-with B 的获取操作 |
| sequenced-before | 同一线程内语句的先后顺序 |
| 修改顺序 | 所有线程对同一原子变量的写入达成一致的全序 |

**happens-before 是传递的**：如果 A happens-before B，B happens-before C，则 A happens-before C。

**建立 happens-before 的关键**：release 操作 synchronizes-with acquire 操作——写线程在 release 前的所有写，对读到该值的读线程可见。

---

## 新手要点（和 C 的区别）

- **C11 也有内存模型**：C11 引入了 `_Atomic` 和内存序，和 C++11 的 `std::atomic` 一致。但 C 程序员通常不接触这些——大多数 C 代码用 pthread mutex 隐式提供内存屏障。
- **为什么需要内存模型**：CPU 和编译器会重排指令——没有内存模型，你无法保证"先写 data 再写 ready"的顺序。内存模型定义了哪些重排允许、哪些不允许。

---

## HFT 关联

- **理解 happens-before 是写无锁代码的前提**：HFT SPSC 无锁队列靠 release-acquire 建立 happens-before，保证消费者读到数据后才读序列号。

---

## 自测题

1. happens-before 和 synchronizes-with 的关系是什么？
2. release-acquire 如何建立 happens-before？
3. 为什么需要内存模型？没有它会怎样？

---

## 参考与延伸

- 下一节：[5.2 六种内存序](02-memory-orders.md)
- 回到：[第 5 章](README.md)
