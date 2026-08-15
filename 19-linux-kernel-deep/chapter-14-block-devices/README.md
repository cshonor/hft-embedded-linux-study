# Ch 14 块设备驱动 · Block Device Drivers

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **⚪ 选读**  
> 磁盘 I/O — 块层、`bio`、电梯调度、DMA 完成

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **单队列块层** | **multiqueue (blk-mq)** 取代单队列 | [Multiqueue block layer](https://lwn.net/Articles/552904/) |
| `request_queue` 单队列 | 改为 per-CPU 软件队列 + 硬件队列 | [Block I/O latency controller](https://lwn.net/Articles/716107/) |
| I/O 调度器 | deadline/cfq 被替换为 mq-deadline/kyber/none | [Block layer multi-queue design](https://docs.kernel.org/block/blk-mq.html) |

> **原则**：块层从单队列到 blk-mq 是架构级重构。ULK3 的块设备章节几乎全部过时，务必查 blk-mq 文档。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 层次结构 | [notes/section-2-块设备层次结构.md](./notes/section-2-块设备层次结构.md) |
| 3. 扇区块与段 | [notes/section-3-扇区块与段.md](./notes/section-3-扇区块与段.md) |
| 4. 通用块层与bio | [notes/section-4-通用块层与bio.md](./notes/section-4-通用块层与bio.md) |
| 5. IO调度程序 | [notes/section-5-IO调度程序.md](./notes/section-5-IO调度程序.md) |
| 6. 驱动与中断 | [notes/section-6-块设备驱动与中断.md](./notes/section-6-块设备驱动与中断.md) |

---

## 相关

- 上一章：[chapter-13-io-architecture/](../chapter-13-io-architecture/)
- 下一章：[chapter-15-page-cache/](../chapter-15-page-cache/)
- 衔接：[Ch 12 VFS](../chapter-12-VFS/) · [Ch 13 DMA/IRQ](../chapter-13-io-architecture/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
