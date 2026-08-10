# 第 16 章 · 缓存一致性

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **选读**

---

## 本章定位

本章详细总结见 [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)。

| | |
|---|---|
| **阅读标签** | **选读**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | 树莓派 4B / **QEMU ARM64**（官方仓库 [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)） |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch16 完整总结 · 缓存一致性** |
| [notes/01-mesi.md](./notes/01-mesi.md) | §16.1 MESI 协议 |
| [notes/02-false-sharing.md](./notes/02-false-sharing.md) | §16.2 伪共享（False Sharing） |
| [notes/03-dma-coherency.md](./notes/03-dma-coherency.md) | §16.3 DMA 一致性 |
| [notes/04-self-modifying-code.md](./notes/04-self-modifying-code.md) | §16.4 自修改代码 |
| [notes/05-lab.md](./notes/05-lab.md) | §16.5 实验要点 |
| [notes/06-pitfalls.md](./notes/06-pitfalls.md) | §16.6 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 15](../chapter-15-cache-basics/) · 下一章 [Ch 17](../chapter-17-tlb-management/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
