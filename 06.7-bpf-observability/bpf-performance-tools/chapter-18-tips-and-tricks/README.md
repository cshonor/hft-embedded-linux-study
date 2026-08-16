# Ch 18 技巧与常见问题 · Tips, Tricks, and Common Problems

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**（生产用 BPF **强烈建议精读**）

> 本章定位：**全书压轴心法** — 无新工具，总结 **开销模型、采样技巧、排障方法论、六大常见坑**。Brendan Gregg 多年性能工程经验的浓缩。  
> **HFT：** 在 tick 核上跑 BPF 前 **必读** — **频率 × 动作 / CPU 数**、**99Hz 采样**、**帧指针/符号**、**勿 trace 追踪器自身**。与 [Ch 2](../chapter-02-technology-background/)、[Ch 12](../chapter-12-languages/)、[Ch 13 § libc 帧指针](../chapter-13-applications/) 闭环。  
> **上一章：** [chapter-17-其他BPF工具.md](../chapter-17-other-tools/) · **附录：** [appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)

---

## 原书真实小节 → 笔记映射

| 原书小节 | 笔记 |
|----|------|
| 18.1 典型事件的频率和额外开销（表 18-1 / 表 18-2） | [notes/section-1-事件频率与开销.md](./notes/section-1-事件频率与开销.md) |
| 18.2 以 49Hz 或 99Hz 为采样频率 / 18.3 黄猪和灰鼠 / 18.4 开发目标软件 / 18.5 学习系统调用 / 18.6 保持简单 | [notes/section-2-建议与技巧.md](./notes/section-2-建议与技巧.md) |
| 18.7 事件缺失 / 18.8 调用栈缺失（18.8.1 修复损坏的调用栈） | [notes/section-3-事件缺失与调用栈缺失.md](./notes/section-3-事件缺失与调用栈缺失.md) |
| 18.9 打印时符号缺失（JIT / ELF）/ 18.10 跟踪时函数缺失（内联/TCO/链接） | [notes/section-4-符号缺失与函数缺失.md](./notes/section-4-符号缺失与函数缺失.md) |
| 18.11 反馈回路 / 18.12 被丢掉的事件 | [notes/section-5-反馈回路与被丢事件.md](./notes/section-5-反馈回路与被丢事件.md) |

---

## 大白话

> 全书压轴心法

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **开销公式**刻进 runbook — **频率** 是第一旋钮。
- [ ] **99Hz**— 所有 CPU 剖析默认。
- [ ] **黄猪/灰鼠**— 内核函数探索的 **实验科学** 法。
- [ ] **帧指针 + 符号**— 比多学两个工具更重要。
- [ ] **简单**— 一个假设一个脚本。
- [ ] **反馈循环**— 生产事故常见自毁类型。

---

## 相关章节

- 上一章：[chapter-17-其他BPF工具.md](../chapter-17-other-tools/)
- 附录 A：[appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)
- 技术地基：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- 语言/符号：[chapter-12-语言.md](../chapter-12-languages/)
- libc 断栈：[chapter-13-应用程序.md](../chapter-13-applications/)
- BCC 调试：[chapter-04-BCC.md](../chapter-04-bcc/)
- 方法论：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 基准：[chapter-12-benchmarking](../../../06.6-systems-performance/chapter-12-benchmarking/)
