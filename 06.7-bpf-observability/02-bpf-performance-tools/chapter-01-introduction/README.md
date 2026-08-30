# Ch 1 引言 · Introduction

> **BPF Performance Tools（《BPF之巅》中文版）** · Brendan Gregg · **精读 🔴**
> **底本：** 《BPF之巅》中文版 1–15 页（孙宇聪/吕宏利/刘晓舟 译）——笔记基于 OCR 底本逐节精读，[全书真实目录](../BOOK-TOC.md)

> 本章定位：**全书导论** — 术语（跟踪/采样/可观测性）、前端格局（BCC/bpftrace/ply）、两个上手工具（execsnoop/biolatency）与两个真实排障故事、插桩选型（动态 kprobes/uprobes vs 静态 tracepoint/USDT）、同一工具的两副面孔（bpftrace 版 opensnoop vs BCC 版）。技术细节在 [Ch 2](../chapter-02-technology-background/)；BCC / bpftrace 专章见 [Ch 4](../chapter-04-bcc/) · [Ch 5](../chapter-05-bpftrace/)。  
> **HFT：** 生产裸机把 **BCC 预制工具 + bpftrace 即兴脚本** 当作与 `perf` 并列的标配 — 本章建立「该用哪条链、能解决什么盲区」的地图。  
> **SysPerf 对照：** [06.6-Systems-Performance Ch 15 BCC/bpftrace](../../../06.6-systems-performance/chapter-15-bpf/) · [Ch 4 观测工具](../../../06.6-systems-performance/chapter-04-observability-tools/)

---

## 小节笔记（按原书 1.1–1.10 划分）

| 节 | 原书标题 | 笔记 |
|----|---------|------|
| 1.1 | BPF 和 eBPF 是什么 | [notes/section-1-BPF和eBPF是什么.md](./notes/section-1-BPF和eBPF是什么.md) |
| 1.2 | 跟踪、嗅探、采样、剖析和可观测性 | [notes/section-2-跟踪嗅探采样剖析与可观测性.md](./notes/section-2-跟踪嗅探采样剖析与可观测性.md) |
| 1.3 | BCC、bpftrace 和 IOVisor | [notes/section-3-BCC-bpftrace与IOVisor.md](./notes/section-3-BCC-bpftrace与IOVisor.md) |
| 1.4 | 初识 BCC：快速上手（execsnoop/biolatency 两案例） | [notes/section-4-初识BCC快速上手.md](./notes/section-4-初识BCC快速上手.md) |
| 1.5 | BPF 跟踪的能见度（软件栈全景图 + 传统工具对照） | [notes/section-5-BPF跟踪的能见度.md](./notes/section-5-BPF跟踪的能见度.md) |
| 1.6 | 动态插桩：kprobes 和 uprobes | [notes/section-6-动态插桩kprobes与uprobes.md](./notes/section-6-动态插桩kprobes与uprobes.md) |
| 1.7 | 静态插桩：tracepoint 和 USDT | [notes/section-7-静态插桩tracepoint与USDT.md](./notes/section-7-静态插桩tracepoint与USDT.md) |
| 1.8 | 初识 bpftrace：跟踪 open() | [notes/section-8-初识bpftrace跟踪open.md](./notes/section-8-初识bpftrace跟踪open.md) |
| 1.9 | 再回到 BCC：跟踪 open() | [notes/section-9-再回到BCC跟踪open.md](./notes/section-9-再回到BCC跟踪open.md) |
| 1.10 | 小结 + 坑点/HFT/自测 | [notes/section-10-小结坑点HFT与自测.md](./notes/section-10-小结坑点HFT与自测.md) |

---

## 本章 Checklist

- [ ] **BPF = 事件发生时在内核运行小程序** — 指令集 + 存储对象 + 辅助函数；验证器保安全不保逻辑
- [ ] **跟踪 ⊂ 可观测性**；benchmark 不属于可观测性（会改变系统状态）
- [ ] **BCC 70+ 工具是起点，bpftrace 是定制问答**；ply 面向嵌入式
- [ ] **execsnoop 抓短命进程**（业务负载画像）；**biolatency 看分布形态**（双峰/离群点），不看平均
- [ ] **插桩选型：先静态（tracepoint/USDT）后动态（kprobe/uprobe）**
- [ ] **跟踪系统调用家族先列变体**：`bpftrace -l 'sys_enter_open*'`（openat 才是主力）

---

## 相关章节

- 下一章：[chapter-02-技术背景](../chapter-02-technology-background/)
- BCC 专章：[chapter-04-BCC](../chapter-04-bcc/) · bpftrace 专章：[chapter-05-bpftrace](../chapter-05-bpftrace/)
- 附录 A 单行命令：[appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)
