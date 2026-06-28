## 1. 本章定位 · 系统全景图

> **ULK Ch 1 Introduction** · 宏观背景，**不涉及底层源码** — 为后续内存、调度、中断等章打地基

第一章是一张 **「系统全景图」**：Unix/Linux 内核长什么样、各子系统如何衔接。后续章节会对每个主题做**源码级**深潜。

| 小节 | 主题 | 笔记 |
|------|------|------|
| 2 | Linux 与其他类 Unix 内核 | [section-2-Linux与Unix比较.md](./section-2-Linux与Unix比较.md) |
| 3 | 基本操作系统概念 | [section-3-基本操作系统概念.md](./section-3-基本操作系统概念.md) |
| 4 | Unix 文件系统概述 | [section-4-Unix文件系统概述.md](./section-4-Unix文件系统概述.md) |
| 5 | Unix 内核概述 | [section-5-Unix内核概述.md](./section-5-Unix内核概述.md) |

**全书组件 ↔ 章节速查** → 见 [section-5](./section-5-Unix内核概述.md#后续章节索引) 末尾索引表

---

### 在本仓库 Linux 链上的位置

```
05 LKD   … 子系统做什么
06 ULK   … 代码里长什么样（Ch 1 全景 → Ch 2+ 深潜）
07 Gorman … 虚拟内存专著
08 TLPI  … 用户态 API
```

← [Ch 1 导读](../README.md) · 下一节 [2. Linux 与 Unix 比较](./section-2-Linux与Unix比较.md)
