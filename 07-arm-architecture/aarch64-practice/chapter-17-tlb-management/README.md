# 第 17 章 · TLB管理

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch17 完整总结 · TLB 管理** |
| [notes/01-tlb-basics.md](./notes/01-tlb-basics.md) | §17.1 TLB 基本概念 |
| [notes/02-asid.md](./notes/02-asid.md) | §17.2 ASID（Address Space ID） |
| [notes/03-tlb-flush.md](./notes/03-tlb-flush.md) | §17.3 TLB 刷新指令 |
| [notes/04-bbm.md](./notes/04-bbm.md) | §17.4 BBM（Break-Before-Make） |
| [notes/05-tlb-scenarios.md](./notes/05-tlb-scenarios.md) | §17.5 内核 TLB 维护场景 |
| [notes/06-lab.md](./notes/06-lab.md) | §17.6 实验要点 |
| [notes/07-pitfalls.md](./notes/07-pitfalls.md) | §17.7 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 16](../chapter-16-cache-coherency/) · 下一章 [Ch 18](../chapter-18-memory-barriers/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
