# Ch 1 Linux 内核简介 · Introduction to the Linux Kernel

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：把 **Linux 内核** 放进 **Unix 史** 与 **OS/内核边界** 里 — 后续 Ch 4 调度、Ch 7 中断、Ch 5 syscall 的地图。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① Unix 史** | Ritchie/Thompson · 1969 | 简洁 · 一切皆文件 · C · fork |
| **② Linux 诞生** | Linus 1991 | GPL · 协作开发 |
| **③ OS 与内核** | kernel vs user space | **syscall / 中断** |
| **④ 宏内核 vs 微内核** | Linux 务实混合 | 模块 · 抢占 · SMP |
| **⑤ 版本号** | 稳定 vs 开发 | **偶数次版本 = 稳定** |
| **⑥ 社区** | LKML | 读代码 + 改代码 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| Unix 的历史 | [notes/section-1.1-Unix-的历史.md](./notes/section-1.1-Unix-的历史.md) |
| Linux 的诞生 | [notes/section-1.2-Linux-的诞生.md](./notes/section-1.2-Linux-的诞生.md) |
| 操作系统与内核概述 | [notes/section-1.3-操作系统与内核概述.md](./notes/section-1.3-操作系统与内核概述.md) |
| Linux 与经典 Unix 内核对比 | [notes/section-1.4-Linux-与经典-Unix-内核对比.md](./notes/section-1.4-Linux-与经典-Unix-内核对比.md) |
| Linux 内核版本 | [notes/section-1.5-Linux-内核版本.md](./notes/section-1.5-Linux-内核版本.md) |
| 内核开发社区 | [notes/section-1.6-内核开发社区.md](./notes/section-1.6-内核开发社区.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| Unix 为何成功？ | **简洁 syscall · 一切皆文件 · C 可移植 · fork/IPC** |
| Linux 何不同？ | **1991 学生项目 → GPL 协作宏内核** |
| 内核 vs app？ | **kernel-space / user-space** · **syscall vs 中断** |
| Linux 架构？ | **宏内核 + 模块 + 抢占 + SMP** |
| 稳定版？ | 传统上 **次版本偶数** |
| 怎么学内核？ | **LKML + 读改源码** |

---

## 本章学习目标 · 自检

- [ ] 能说出 **syscall** 与 **中断** 各解决哪方向通信
- [ ] 区分 **宏内核** 与 **微内核** · Linux 折中在哪
- [ ] 知道 **GPL 2.0** 与 Linux 分发方式
- [ ] 与 [01 自制 OS](../../09-system-low-level-hands-on/01-30days-os/) 的 **Ring0/INT 0x40** 对照看一眼

---

## 相关章节

- 下一章：[../chapter-02-getting-started/](../chapter-02-getting-started/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
