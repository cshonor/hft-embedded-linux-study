## 12.4 多线程程序中的共享变量

> ↔ [Hennessy §5.3 伪共享](../../../17-computer-architecture/chapter-05-thread-level-parallelism/notes/section-5.3-性能分析与伪共享.md)


> **Ch12 §12.4** · [章导读](../README.md) · 上节 [§12.3 ←](./section-12.3-基于线程的并发编程.md) · 下节 [§12.5 →](./section-12.5-信号量与预线程化.md)

---

← [本章导读](../README.md)

---

### 共享变量与竞态条件

- **共享变量：** 多线程可访问的变量（全局变量、堆上的数据、通过指针传递的栈变量）
- **竞态条件（race condition）：** 多线程并发读写共享变量，结果依赖执行顺序

**经典示例 — counter++ 不是原子操作：**

```c
// 看似一条语句，实际三步：
// 1. load  counter 到寄存器
// 2. add   寄存器 +1
// 3. store 寄存器回 counter
// 两个线程同时执行 → 可能丢失一次增量
```

| 变量类型 | 是否共享 | 需要同步？ |
|----------|----------|-----------|
| 局部变量（栈） | 否（每线程独立栈） | 否 |
| 全局变量 | 是 | 是 |
| 堆变量（malloc） | 是（若多线程访问） | 是 |
| register 变量 | 否（每线程独立寄存器） | 否 |

**HFT：** 热路径尽量不共享（每线程独立数据），必须共享时用无锁结构（SPSC 队列）。

### 常见陷阱
1. **共享变量需同步，声明 volatile 不够** — volatile 只防编译器优化（不缓存到寄存器），不防 CPU 乱序和竞态
2. **counter++ 不是原子操作** — load-add-store 三步，两线程并发可能丢失一次增量
3. **栈变量默认不共享** — 但通过指针传递给其他线程后就变成共享变量，需同步

### 自测题

<details>
<summary>Q1: counter++ 在汇编层面是几条指令？为什么不是原子操作？</summary>

三条指令：load（从内存读到寄存器）、add（寄存器+1）、store（写回内存）。两线程同时 load 可能读到相同值，各自 +1 后写回，结果只增加 1 而非 2。

</details>

<details>
<summary>Q2: volatile 能解决竞态条件吗？为什么？</summary>

不能。volatile 只防止编译器将变量缓存到寄存器（保证每次读写访问内存），但不防止 CPU 指令乱序执行，也不保证 load-add-store 的原子性。需要 mutex/atomic 解决。

</details>

<details>
<summary>Q3: 哪些变量是线程私有的？哪些是共享的？</summary>

私有：局部变量（栈）、register 变量、线程局部存储（TLS）。共享：全局变量、static 变量、堆变量（malloc）。注意：栈变量通过指针传给其他线程后变为共享。

</details>

<details>
<summary>Q4: HFT 热路径如何处理共享变量？</summary>

最佳：不共享（每线程独立数据，thread-local）。必须共享时：1) SPSC 无锁队列（单生产者单消费者，无竞争）；2) 原子操作（std::atomic with relaxed/release/acquire）；3) 避免互斥锁（不确定性延迟）。

</details>

---

← [§12.3 ←](./section-12.3-基于线程的并发编程.md) · [本章导读](../README.md) · [§12.5 →](./section-12.5-信号量与预线程化.md)
