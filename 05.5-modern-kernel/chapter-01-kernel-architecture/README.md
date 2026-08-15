# Ch1 内核架构概述

> 来源: Bootlin Kernel Training
> 对标旧书: ULK3 Ch1 / LKD3 Ch1-2 (已过时)

内核空间与用户空间隔离、6.x 子系统变化、源码组织与编译。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 1.1 内核空间 vs 用户空间与子系统概览 | `notes/01-kernel-space-vs-user-space.md` |
| 1.2 内核源码组织与编译 | `notes/02-kernel-source-organization.md` |

---

## HFT 关联

理解内核子系统全景是 HFT 调优的基础。6.x 的 maple tree、folio、blk-mq 等变化直接影响内核性能特征。
