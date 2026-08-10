# 第 14 章 · 内存管理

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
| [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md) | **Ch14 完整总结 · 页表与 MMU** |
| [notes/01-va-space.md](./notes/01-va-space.md) | §14.1 虚拟地址空间 |
| [notes/02-four-level-page-table.md](./notes/02-four-level-page-table.md) | §14.2 四级页表 |
| [notes/03-descriptor-format.md](./notes/03-descriptor-format.md) | §14.3 页表项（Descriptor）格式 |
| [notes/04-memory-attributes.md](./notes/04-memory-attributes.md) | §14.4 内存属性 |
| [notes/05-access-permission.md](./notes/05-access-permission.md) | §14.5 访问权限（AP） |
| [notes/06-enable-mmu.md](./notes/06-enable-mmu.md) | §14.6 开 MMU 流程 |
| [notes/07-lab.md](./notes/07-lab.md) | §14.7 实验要点 |
| [notes/08-pitfalls.md](./notes/08-pitfalls.md) | §14.8 易错点清单 |

---

## 本章 Checklist

- [x] 读完原书对应章
- [x] 完成书中实验（若有）
- [x] 在 `notes/` 记录可复述要点

---

← [Ch 13](../chapter-13-gic-v2/) · 下一章 [Ch 15](../chapter-15-cache-basics/) · [OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)
