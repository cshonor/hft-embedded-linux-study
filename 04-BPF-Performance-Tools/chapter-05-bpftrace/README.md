# Ch 5 bpftrace · bpftrace

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**ad hoc 排障与短脚本的入口** — 若 [Ch 4 BCC](../chapter-04-bcc/) 是写复杂工具、守护进程的 **重型武器**，bpftrace 则适合 **临时验证假设、单行命令 (one-liners)、几十行短脚本**。语法类似 **awk + C**，大幅降低 eBPF 门槛。  
> **HFT：** BCC runbook 定方向后，用 bpftrace **几分钟内** 验证「是不是这个 syscall / 这个 PID / 这条栈」— 比起完整 BCC Python 项目快一个数量级；仍须遵守 **内核聚合、短窗口** 原则。  
> **上一章：** [chapter-04-BCC.md](../chapter-04-bcc/) · **下一章：** [chapter-06-CPU.md](../chapter-06-cpus/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 bpftrace 是什么 | [notes/section-1-bpftrace是什么.md](./notes/section-1-bpftrace是什么.md) |
| 2 核心架构与编译流程 | [notes/section-2-核心架构与编译流程.md](./notes/section-2-核心架构与编译流程.md) |
| 3 全栈事件源 (Probes) | [notes/section-3-全栈事件源.md](./notes/section-3-全栈事件源.md) |
| 4 编程语法结构 | [notes/section-4-编程语法结构.md](./notes/section-4-编程语法结构.md) |
| 5 三大变量类型 | [notes/section-5-三大变量类型.md](./notes/section-5-三大变量类型.md) |
| 6 Map 聚合函数 | [notes/section-6-Map聚合函数.md](./notes/section-6-Map聚合函数.md) |
| 7 常用内置函数 | [notes/section-7-常用内置函数.md](./notes/section-7-常用内置函数.md) |
| 8 控制流限制 | [notes/section-8-控制流限制.md](./notes/section-8-控制流限制.md) |
| 9 调试与排障 | [notes/section-9-调试与排障.md](./notes/section-9-调试与排障.md) |
| 10 经典 One-Liners 速览 | [notes/section-10-经典One-Liners速览.md](./notes/section-10-经典One-Liners速览.md) |
| 11 Part II 预告（Ch 6+） | [notes/section-11-PartII预告Ch6.md](./notes/section-11-PartII预告Ch6.md) |

---

## 大白话

> ad hoc 排障与短脚本的入口

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **bpftrace = 假设验证加速器**— BCC runbook 之后、改代码之前的 **5 分钟层**。
- [ ] **语法核心：**`probe /filter/ { @map = agg(); }` — 内置变量 + `$` 临时 + `@` 聚合。
- [ ] **高频事件只用 Map 函数**（`count`、`hist`）— `printf` 仅低频或调试。
- [ ] **`kstack`/`ustack` + `profile`**与 BCC `stackcount`/`profile` 同族 — 火焰图仍见 [Ch 2](../chapter-02-technology-background/)。
- [ ] **无无限循环**— 用多探针 + map 表达状态；`unroll(N)` 有界展开。

---

## 相关章节

- 上一章：[chapter-04-BCC.md](../chapter-04-bcc/)
- 下一章：[chapter-06-CPU.md](../chapter-06-cpus/)
- 技术地基：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- 方法论与清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- 附录 A 单行命令：[appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)
- 附录 B 备忘单：[appendix-B-bpftrace备忘单.md](../appendix-B-bpftrace备忘单.md)
- SysPerf bpftrace：[appendix-C-bpftrace单行命令](../../03-Systems-Performance-2nd/appendix-C-bpftrace单行命令.md)
