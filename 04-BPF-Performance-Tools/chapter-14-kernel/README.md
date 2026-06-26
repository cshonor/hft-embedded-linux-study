# Ch 14 内核 · Kernel

> **BPF Performance Tools** · Brendan Gregg · **精读 🟡**（内核开发者 🔴）

> 本章定位：**内核本身作为分析目标** — Ch 6–13 借内核观测 **应用**；本章深入 **调度唤醒链、内核锁、Slab/页分配、工作队列**。对 **内核开发者** 极有用；HFT 共置机 **incident 深潜** 时用于「系统卡顿但应用说不清」类问题。  
> **HFT：** 常态 **选读**；`offwaketime` 解阻塞链、`kmem`/`slabratetop` 查内核内存、`mlock` 查内核 mutex。优先 **tracepoint** 而非脆弱 kprobe。与 [05-LKD](../05-Linux-Kernel-Development/) · [07-Gorman](../07-Linux-Virtual-Memory-Manager/) 对照。  
> **上一章：** [chapter-13-应用程序.md](../chapter-13-applications/) · **下一章：** [chapter-15-容器.md](../chapter-15-containers/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章 vs 前几章 | [notes/section-1-本章vs前几章.md](./notes/section-1-本章vs前几章.md) |
| 2 内核基础知识 (Kernel Fundamentals) | [notes/section-2-内核基础知识.md](./notes/section-2-内核基础知识.md) |
| 3 传统内核分析工具 | [notes/section-3-传统内核分析工具.md](./notes/section-3-传统内核分析工具.md) |
| 4 调度与唤醒 (Scheduler & Wakeups) | [notes/section-4-调度与唤醒.md](./notes/section-4-调度与唤醒.md) |
| 5 内核锁分析 | [notes/section-5-内核锁分析.md](./notes/section-5-内核锁分析.md) |
| 6 内核内存分析 | [notes/section-6-内核内存分析.md](./notes/section-6-内核内存分析.md) |
| 7 工作队列 — `workq` | [notes/section-7-工作队列workq.md](./notes/section-7-工作队列workq.md) |
| 8 内核追踪的挑战 | [notes/section-8-内核追踪的挑战.md](./notes/section-8-内核追踪的挑战.md) |
| 9 工具选型速查 | [notes/section-9-工具选型速查.md](./notes/section-9-工具选型速查.md) |

---

## 大白话

> 内核本身作为分析目标

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **常态少读**— 除非 **整机卡顿** 且 Ch 6/13/10 无法闭环。
- [ ] **`offwaketime`**— 比单独 `offcputime` 多 **唤醒者** 半条链；共置机 **I/O 完成路径** 排查利器。
- [ ] **内核内存**— `slabratetop` + `kmem` 查 **驱动/内核泄漏**；与用户态 `memleak`（Ch 7）分工。
- [ ] **`mlock` vs `pmlock`**— 内核 mutex vs **pthread**；`syscount` 见 futex 时先 Ch 13。
- [ ] **自旋锁用 profile，勿 kretprobe**。
- [ ] **tracepoint > kprobe**— 内核升级后 runbook 仍可用。

---

## 相关章节

- 上一章：[chapter-13-应用程序.md](../chapter-13-applications/)
- 下一章：[chapter-15-容器.md](../chapter-15-containers/)
- Off-CPU：[chapter-06-CPU.md](../chapter-06-cpus/) · [chapter-13-应用程序.md](../chapter-13-applications/)
- 用户态内存：[chapter-07-内存.md](../chapter-07-memory/)
- Ftrace：[chapter-14-ftrace](../../03-Systems-Performance-2nd/chapter-14-ftrace/)
- LKD：[05-Linux-Kernel-Development](../05-Linux-Kernel-Development/)
- Gorman：[07-Linux-Virtual-Memory-Manager](../07-Linux-Virtual-Memory-Manager/)
