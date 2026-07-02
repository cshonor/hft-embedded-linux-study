# Ch 2 存储器层次结构设计 · Memory Hierarchy Design

> **Computer Architecture 6th** · Hennessy & Patterson · **精读 🔴**

> 本章定位：**局部性的工程落地** — 用 SRAM/DRAM/HBM/Flash 等技术叠出层次结构，并用命中时间、未命中率、未命中惩罚、带宽与功耗等 **可量化指标** 优化缓存与主存。HFT 热路径的绝大部分「莫名变慢」，最终都能追溯到这一章。

**核心问题：** 处理器越来越快、DRAM 相对越来越慢 — 如何用 **时间/空间局部性** 掩盖延迟？软件与硬件各能做什么？

**平均访存时间（AMAT）直觉：**

\[
\text{AMAT} = \text{命中时间} + \text{未命中率} \times \text{未命中惩罚}
\]

优化缓存 = 同时动这五维：**命中时间、未命中率、未命中惩罚、带宽、功耗**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 2.1 引言与层次结构 | [notes/section-2.1-引言与存储器层次.md](./notes/section-2.1-引言与存储器层次.md) |
| 2.2 存储器技术与优化 | [notes/section-2.2-存储器技术与优化.md](./notes/section-2.2-存储器技术与优化.md) |
| 2.3 缓存性能十项高级优化 | [notes/section-2.3-缓存性能十项高级优化.md](./notes/section-2.3-缓存性能十项高级优化.md) |
| 2.4 虚拟内存与虚拟机 | [notes/section-2.4-虚拟内存与虚拟机.md](./notes/section-2.4-虚拟内存与虚拟机.md) |
| 2.5 交叉领域问题 | [notes/section-2.5-交叉领域问题.md](./notes/section-2.5-交叉领域问题.md) |
| 2.6 实例：Cortex-A53 与 Core i7 | [notes/section-2.6-实例分析-Cortex-A53与Core-i7.md](./notes/section-2.6-实例分析-Cortex-A53与Core-i7.md) |
| 2.7 谬误与陷阱 | [notes/section-2.7-谬误与陷阱.md](./notes/section-2.7-谬误陷阱.md) |

---

## HFT 精读捷径

| 本节 | 带走什么 |
|------|----------|
| 2.1–2.2 | **Cache line 64B**、DRAM 带宽 vs 延迟 — 订单簿布局的第一性原理 |
| 2.3 | **分块/预取/false sharing** — 无锁结构、热循环优化的硬件依据 |
| 2.4 | **TLB 缺页、mmap/hugepage** — 实盘内存映射与 NUMA 前置 |
| 2.6 | 服务器 CPU 的 **L3 + 激进预取** vs 移动端省电缓存 — 选型参考 |
| 2.7 | 别用别的程序的 cache 行为外推自己的热路径 |

→ [01-CSAPP Ch6](../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [03-Gorman](../../05-Linux-Virtual-Memory-Manager/) · [Ch5 线程级并行](../chapter-05-thread-level-parallelism/)（false sharing / MESI）

---

## 相关章节

- 上一章：[chapter-01-quantitative-design-fundamentals](../chapter-01-quantitative-design-fundamentals/)
- 下一章：[chapter-03-instruction-level-parallelism](../chapter-03-instruction-level-parallelism/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
