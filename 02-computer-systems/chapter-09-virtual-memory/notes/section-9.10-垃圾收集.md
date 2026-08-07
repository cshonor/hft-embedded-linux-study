## 9.10 垃圾收集（9.10.1–9.10.3）

### 基本概念

- **GC** — 自动回收不可达对象；**Java/Go** 默认有
- **可达性** — 从根（栈、全局）沿指针图遍历

### Mark & Sweep

1. **标记** 所有可达块
2. **清除** 未标记块回空闲链表

### C 的保守 GC

- 把 **栈/寄存器里像指针的位模式** 都当根 — **可能漏标或误留**
- 生产 C/C++ **极少** 用；HFT 用 **RAII / 池 / Rust 所有权**

**HFT：** 理解 GC **停顿 (stop-the-world)** 为何不适合 tick 线程；策略语言选型时考虑 **latency tail**。

→ [Ch 5 应用/GC 对比](../../chapter-05-optimizing-performance/) · [Ch 12 并发](../../chapter-12-concurrent-programming/)

### 常见陷阱

1. **保守 GC 把像指针的位模式都当根，可能误留** — 整数碰巧像地址 → 该回收的块不回收（false positive，不误删）
2. **stop-the-world 停顿对 HFT 致命** — GC 暂停所有应用线程，可能 ms-s 级；tick 线程必须避免
3. **C/C++ 几乎不用 GC，用 RAII/池/Rust 所有权** — GC 的不确定性延迟与 HFT 的确定性要求矛盾

### 自测题

<details>
<summary>Q1: Mark & Sweep 的两个阶段分别做什么？</summary>

Mark：从根（栈、全局变量、寄存器）出发沿指针图遍历，标记所有可达对象。Sweep：扫描整个堆，未标记的块回收加入空闲链表。
</details>

<details>
<summary>Q2: C 的保守 GC（conservative GC）为什么不精确？</summary>

C 没有类型信息，GC 无法区分指针和整数。把栈/寄存器中所有像指针的位模式都当根，可能误留不可达对象（但不误删可达对象）。
</details>

<details>
<summary>Q3: 为什么 HFT 不用 GC？用什么替代？</summary>

GC 的 stop-the-world 停顿不确定（ms-s 级），与 HFT 确定性延迟要求矛盾。替代：RAII（C++ 析构）、对象池（预分配）、Rust 所有权（编译期保证无泄漏/无 UAF）。
</details>

<details>
<summary>Q4: GC 的延迟（latency tail）问题如何影响系统设计？</summary>

即使平均 GC 停顿很短（μs），最坏情况可能很长（s）。系统设计要考虑最坏延迟：tick 线程不分配堆、用 pre-allocated buffer、GC 语言（Java/Go）需调优 GC 参数减少 STW 时间。
</details>

---

← [本章导读](../README.md)
