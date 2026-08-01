# Ch 1 简介 · Introduction

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**

本章 **不深入 VM 技术细节**，而是 **内核开发与源码阅读指南**：怎么拿源码、打补丁、浏览代码、按什么顺序读 `mm/`，以及怎么向社区提交改动。

---

## 本章在全书中的位置

| | 内容 |
|---|------|
| **本章** | 工具链 + 阅读方法论 + 社区流程 |
| **Ch 2 起** | 物理内存、页表、地址空间、slab、回收等 **VM 本体** |
| **附录 A–M** | 与正文对应的 **Code Commentary**（按子系统拆的源码导读） |

> **HFT 读法：** 不必死记工具名；带走两样东西——**(1) 作者推荐的 `mm/` 阅读顺序**；(2) 补丁 / 邮件列表文化。技术细节从 [Ch 2](../../chapter-02-describing-physical-memory/) 精读。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 入门指南 | [notes/section-1-入门指南.md](./notes/section-1-入门指南.md) |
| 2. 源码管理 | [notes/section-2-源码管理.md](./notes/section-2-源码管理.md) |
| 3. 浏览代码 | [notes/section-3-浏览代码.md](./notes/section-3-浏览代码.md) |
| 4. 阅读代码的策略 | [notes/section-4-阅读代码的策略.md](./notes/section-4-阅读代码的策略.md) |
| 5. 提交补丁 | [notes/section-5-提交补丁.md](./notes/section-5-提交补丁.md) |

---

## 相关章节

- 下一章：[../chapter-02-describing-physical-memory/](../chapter-02-describing-physical-memory/)
- 附录 A：[../../appendix-A-简介.md](../../appendix-A-简介.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
