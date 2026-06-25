# Ch 4 BCC · BCC (BPF Compiler Collection)

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**BCC 工具箱使用说明书 + 多用途工具内部逻辑** — BPF 的 **主要前端项目**，含 **70+** 开箱即用性能/排障工具。读懂本章，才能从「跑现成脚本」过渡到「用 BCC 写自己的 BPF 工具」。  
> **HFT：** 生产环境 **内核侧粗筛与下钻** 的主力载体（`runqlat`、`profile`、`tcpretrans` 等多为 BCC 实现）；理解 **单用途 vs 多用途** 与 **内核聚合 vs 逐行打印**，避免在热路径上误用 `trace`。  
> **上一章：** [chapter-03-性能分析.md](../chapter-03-performance-analysis/) · **下一章：** [chapter-05-bpftrace.md](../chapter-05-bpftrace/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 BCC 是什么 | [notes/section-1-BCC是什么.md](./notes/section-1-BCC是什么.md) |
| 2 BCC 架构与特性 | [notes/section-2-BCC架构与特性.md](./notes/section-2-BCC架构与特性.md) |
| 3 单用途 vs 多用途：设计哲学 | [notes/section-3-单用途vs多用途设计哲学.md](./notes/section-3-单用途vs多用途设计哲学.md) |
| 4 四大多用途工具 | [notes/section-4-四大多用途工具.md](./notes/section-4-四大多用途工具.md) |
| 5 规范的工具文档 | [notes/section-5-规范的工具文档.md](./notes/section-5-规范的工具文档.md) |
| 6 BCC 调试与排障 | [notes/section-6-BCC调试与排障.md](./notes/section-6-BCC调试与排障.md) |
| 7 BCC vs bpftrace（预告） | [notes/section-7-BCCvsbpftrace预告.md](./notes/section-7-BCCvsbpftrace预告.md) |

---

## 大白话

> BCC 工具箱使用说明书 + 多用途工具内部逻辑

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **BCC = 生产级工具箱 + 可编程前端**— Ch 3 清单里的工具大多源于此生态。
- [ ] **高频用聚合**（`funccount`、`argdist`、`runqlat` 类）— **低频用 `trace`**。
- [ ] **`stackcount` + 火焰图**找「哪条路径触发了异常 syscall/锁」；**`profile`** 找 CPU 热点 — 别混用场景。
- [ ] **每个工具先读 man + examples**— 开销与字段含义比背命令重要。

---

## 相关章节

- 上一章：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- 下一章：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- 技术地基：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- BCC 自研：[appendix-C-BCC工具开发.md](../appendix-C-BCC工具开发.md)
- SysPerf BPF 章：[chapter-15-bpf](../../02-Systems-Performance-2nd/chapter-15-bpf/)
- 网络工具实践：[chapter-10-网络.md](../chapter-10-networking/)
