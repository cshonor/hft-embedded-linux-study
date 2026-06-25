# Ch 1 简介 · Introduction

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**全书导论** — 术语、工具链第一印象、两个「现场排障」故事，以及 BPF 相对传统观测的 **可见性** 与 **插桩** 分类。技术细节在 [Ch 2](../chapter-02-technology-background/)；BCC / bpftrace 专章见 [Ch 4](../chapter-04-bcc/) · [Ch 5](../chapter-05-bpftrace/)。  
> **HFT：** 生产裸机把 **BCC 预制工具 + bpftrace 即兴脚本** 当作与 `perf` 并列的标配 — 本章建立「该用哪条链、能解决什么盲区」的地图。  
> **SysPerf 对照：** [02-SysPerf Ch 15 BCC/bpftrace](../../02-Systems-Performance-2nd/chapter-15-bpf/) · [Ch 4 观测工具](../../02-Systems-Performance-2nd/chapter-04-observability-tools/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 基础概念 | [notes/section-1-基础概念.md](./notes/section-1-基础概念.md) |
| 2 核心前端：为何需要 BCC / bpftrace | [notes/section-2-核心前端为何需要BCCbpftrace.md](./notes/section-2-核心前端为何需要BCCbpftrace.md) |
| 3 BCC 工具初探 · 快速排障 | [notes/section-3-BCC工具初探快速排障.md](./notes/section-3-BCC工具初探快速排障.md) |
| 4 BPF 的可见性 (Visibility) | [notes/section-4-BPF的可见性.md](./notes/section-4-BPF的可见性.md) |
| 5 动态插桩 vs 静态插桩 | [notes/section-5-动态插桩vs静态插桩.md](./notes/section-5-动态插桩vs静态插桩.md) |
| 6 bpftrace 与 BCC 演示 · 追 `open()` | [notes/section-6-bpftrace与BCC演示追open.md](./notes/section-6-bpftrace与BCC演示追open.md) |

---

## 大白话

> 全书导论

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **BPF ≠ 只做网络过滤**— eBPF 是 **内核可编程观测 +（另册）XDP/tc 数据面**。
- [ ] **先 BCC 标准工具，再 bpftrace 定制**— 与 [SysPerf 15.1.7](../../02-Systems-Performance-2nd/chapter-15-bpf/notes/section-15.1.7-BCC-vs-bpftrace.md) 一致。
- [ ] **Tracing 补 Sampling**— `profile` 找 CPU 热点；`runqlat` / `biolatency` / `tcpretrans` 找 **延迟与长尾**。
- [ ] **Tracepoint > kprobe**（能用时）— 内核升级时脚本更稳。

---

## 相关章节

- 下一章：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- BCC 专章：[chapter-04-BCC.md](../chapter-04-bcc/)
- bpftrace 专章：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- 附录 A 单行命令：[appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)
