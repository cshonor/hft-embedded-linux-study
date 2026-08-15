# Ch 2 技术背景 · Technology Background

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**全书技术地基** — eBPF VM、Map、辅助函数、栈遍历、火焰图、四类插桩（k/u probe、Tracepoint/USDT）、PMC/perf。后续 BCC/bpftrace 工具都建在这些组件之上。  
> **HFT：** 读懂本章才能判断「这条 probe 为什么贵」「火焰图为什么缺帧」「换内核后脚本为何挂」— 避免在生产热路径上误用 per-event 输出。  
> **上一章：** [chapter-01-简介.md](../chapter-01-introduction/) · **下一章：** [chapter-03-性能分析.md](../chapter-03-performance-analysis/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 BPF 与 eBPF | [notes/section-1-BPF与eBPF.md](./notes/section-1-BPF与eBPF.md) |
| 2 堆栈追踪遍历 (Stack Trace Walking) | [notes/section-2-堆栈追踪遍历.md](./notes/section-2-堆栈追踪遍历.md) |
| 3 火焰图 (Flame Graphs) | [notes/section-3-火焰图.md](./notes/section-3-火焰图.md) |
| 4 动态插桩：kprobes 与 uprobes | [notes/section-4-动态插桩kprobes与uprobes.md](./notes/section-4-动态插桩kprobes与uprobes.md) |
| 5 静态插桩：Tracepoints 与 USDT | [notes/section-5-静态插桩Tracepoints与USDT.md](./notes/section-5-静态插桩Tracepoints与USDT.md) |
| 6 PMCs 与 perf_events | [notes/section-6-PMCs与perf_events.md](./notes/section-6-PMCs与perf_events.md) |
| 7 技术组件地图（后文工具如何挂接） | [notes/section-7-技术组件地图后文工具如何挂接.md](./notes/section-7-技术组件地图后文工具如何挂接.md) |

---

## 大白话

> 全书技术地基

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **内核聚合、用户展示**— 热路径上只 export 统计，不 export 原始事件流。
- [ ] **Tracepoint 优先，kprobe 兜底**— 内核升级维护成本差一个数量级。
- [ ] **uprobe 远离高频函数**— `malloc`/每 tick 路径用 **采样** 或 **USDT**。
- [ ] **火焰图要栈得先要有帧**— 构建链保留 frame pointer 或配 debuginfo。
- [ ] **CO-RE 是跨内核部署的未来**— 定制工具长期应规划 libbpf + BTF。

---

## 相关章节

- 上一章：[chapter-01-简介.md](../chapter-01-introduction/)
- 下一章：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- BCC：[chapter-04-BCC.md](../chapter-04-bcc/) · bpftrace：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- CPU / PMC 实践：[chapter-06-CPU.md](../chapter-06-cpus/)
- C / CO-RE：[appendix-D-C语言BPF.md](../appendix-D-C语言BPF.md)
