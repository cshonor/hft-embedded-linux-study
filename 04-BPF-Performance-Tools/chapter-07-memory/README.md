# Ch 7 内存 · Memory

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**内存压力与分配路径** — CPU 扩展快于 DRAM 的时代，**内存 I/O** 常是隐性瓶颈。回顾虚拟/物理内存、缺页与回收机制后，介绍用 BPF 把「RSS 涨、OOM、卡顿」定位到 **具体代码路径** 的工具。  
> **HFT：** 热路径应 **预分配 + 池化**，正常交易时很少触发 `memleak`/`drsnoop`；本章主要用于 **共置机内存争抢、泄漏、OOM、swap 误开** 等 incident。与 [Ch 6](../chapter-06-cpus/) `llcstat` / cache 衔接。  
> **上一章：** [chapter-06-CPU.md](../chapter-06-cpus/) · **下一章：** [chapter-08-文件系统.md](../chapter-08-file-systems/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章要回答的问题 | [notes/section-1-本章要回答的问题.md](./notes/section-1-本章要回答的问题.md) |
| 2 内存基础知识 (Background) | [notes/section-2-内存基础知识.md](./notes/section-2-内存基础知识.md) |
| 3 传统内存分析工具 | [notes/section-3-传统内存分析工具.md](./notes/section-3-传统内存分析工具.md) |
| 4 BPF 相对传统工具的优势 | [notes/section-4-BPF相对传统工具的优势.md](./notes/section-4-BPF相对传统工具的优势.md) |
| 5 异常与泄漏排查 | [notes/section-5-异常与泄漏排查.md](./notes/section-5-异常与泄漏排查.md) |
| 6 内存分配与映射 | [notes/section-6-内存分配与映射.md](./notes/section-6-内存分配与映射.md) |
| 7 缺页异常分析 | [notes/section-7-缺页异常分析.md](./notes/section-7-缺页异常分析.md) |
| 8 内存回收与交换 | [notes/section-8-内存回收与交换.md](./notes/section-8-内存回收与交换.md) |
| 9 工具选型速查 | [notes/section-9-工具选型速查.md](./notes/section-9-工具选型速查.md) |
| 10 BPF / bpftrace One-Liners（示意） | [notes/section-10-BPFbpftraceOne-Liners示意.md](./notes/section-10-BPFbpftraceOne-Liners示意.md) |
| 11 与 CPU / 文件系统章节的衔接 | [notes/section-11-与CPU文件系统章节的衔接.md](./notes/section-11-与CPU文件系统章节的衔接.md) |

---

## 大白话

> 内存压力与分配路径

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **热路径设计**应让 Ch 7 工具在常态下 **几乎无事可做** — 池化、预分配、禁 swap。
- [ ] **`memleak` / `drsnoop` 是 incident 工具**— 短窗口、限 PID，勿与低延迟核长期共存。
- [ ] **Direct reclaim**是「内存够但抖」的常见根因 — `drsnoop` 比 `free` 更 actionable。
- [ ] **OOM**用 `oomkill` 找 **触发者**，不是只看被杀进程。
- [ ] **缺页火焰图 (`faults`)**用于冷启动、新库上线 — 非 tick 热路径常态监控。

---

## 相关章节

- 上一章：[chapter-06-CPU.md](../chapter-06-cpus/)
- 下一章：[chapter-08-文件系统.md](../chapter-08-file-systems/)
- 磁盘 I/O：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- 方法论：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 内存：[chapter-07-memory](../../03-Systems-Performance-2nd/chapter-07-memory/)
- CSAPP 虚拟内存：[chapter-09-virtual-memory](../01-CSAPP-3rd/chapter-09-virtual-memory/)
