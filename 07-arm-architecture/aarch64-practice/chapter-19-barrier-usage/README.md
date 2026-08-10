# 第 19 章 · 合理使用内存屏障指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **精读**

---

## 本章定位

本章详细总结见 [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)。

| | |
|---|---|
| **阅读标签** | **精读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | 树莓派 4B / **QEMU ARM64**（官方仓库 [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)） |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch19 完整总结 · 合理使用内存屏障** |
| [notes/01-spinlock.md](./notes/01-spinlock.md) | §19.1 案例一：自旋锁获取/释放 |
| [notes/02-message-passing.md](./notes/02-message-passing.md) | §19.2 案例二：消息传递（邮箱） |
| [notes/03-dma.md](./notes/03-dma.md) | §19.3 案例三：DMA 操作 |
| [notes/04-tlb-maintenance.md](./notes/04-tlb-maintenance.md) | §19.4 案例四：TLB 维护 |
| [notes/05-decision-tree.md](./notes/05-decision-tree.md) | §19.5 屏障选择决策树 |
| [notes/06-hft-spsc.md](./notes/06-hft-spsc.md) | §19.6 HFT 中的屏障使用 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | §19.7 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 18](../chapter-18-memory-barriers/) · 下一章 [Ch 20](../chapter-20-atomic-operations/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
