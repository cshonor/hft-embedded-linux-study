# Ch 2 mbuf 与内存池 · mbuf & Mempool

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **实体书：** [chapter-06-PCIe与包处理IO](./chapter-06-PCIe与包处理IO/) §6 精讲 mbuf/mempool 布局与 Core Cache；先读 [chapter-02-Cache与内存](./chapter-02-Cache与内存/)（大页、NUMA、Cache 对齐）。

<!-- 实验向笔记待补充：rte_mbuf 字段、mempool 创建参数、与 sk_buff 对照 -->

## 相关章节

- 实体书：[chapter-06-PCIe与包处理IO/](./chapter-06-PCIe与包处理IO/) · [chapter-04-同步互斥机制/](./chapter-04-同步互斥机制/)（无锁 ring）
- 上一章：[chapter-02-Cache与内存.md](./chapter-02-Cache与内存.md) · [chapter-03-并行计算.md](./chapter-03-并行计算.md)
- 下一章：[chapter-03-PMD与轮询模式.md](./chapter-03-PMD与轮询模式.md)
- 缓存对照：[01-CSAPP Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/)
