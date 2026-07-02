# Ch 6 PCIe 与包处理 I/O · PCIe & Packet I/O

> **01-Intro-Book** · 《深入浅出 DPDK》第六章 · **🔴 HFT 精读**  
> PCIe TLP/DMA、描述符环、MMIO 批处理、mbuf/mempool

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. PCIe 事务与带宽 | [notes/section-2-PCIe事务与带宽.md](./notes/section-2-PCIe事务与带宽.md) |
| 3. DMA 描述符环形队列 | [notes/section-3-DMA描述符环形队列.md](./notes/section-3-DMA描述符环形队列.md) |
| 4. CPU 与 I/O 协奏优化 | [notes/section-4-CPU与IO协奏优化.md](./notes/section-4-CPU与IO协奏优化.md) |
| 5. PCIe 净荷带宽计算 | [notes/section-5-PCIe净荷带宽计算.md](./notes/section-5-PCIe净荷带宽计算.md) |
| 6. Mbuf 与 Mempool | [notes/section-6-Mbuf与Mempool.md](./notes/section-6-Mbuf与Mempool.md) |
| 7. 小结与索引 | [notes/section-7-小结与索引.md](./notes/section-7-小结与索引.md) |

---

## 相关

- 上一章：[chapter-05-packet-forwarding/](../chapter-05-packet-forwarding/)
- 下一章：[chapter-07-nic-performance-optimization/](../chapter-07-nic-performance-optimization/) · [chapter-03-PMD与轮询模式.md](../chapter-03-PMD与轮询模式.md)
- 对照：[Ch2 Cache/大页](../chapter-02-cache-and-memory/) · [Ch4 无锁 ring](../chapter-04-synchronization/)
