# BPF Performance Tools — Brendan Gregg

**文件夹 03** · 全书 **18 章 + 附录 A–E**（4 Parts）· [返回总清单](../READING-LIST.md#8-bpf-performance-tools--brendan-gregg)

> **文件夹 03** · 紧接 [03-SysPerf](../03-Systems-Performance-2nd/) 阅读。  
> **后续：** `05`–`08` 系统纵深 · [15-HFT](../15-HFT-Low-Latency-Practice/) / [13-Rust](../16-Rust-Quant-Trading-Guide/)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 目录结构

```
chapter-XX-english-slug/
├── README.md      ← 章导读（小节索引、大白话、Checklist）
└── notes/         ← 按原书小节拆分的笔记
```

与 [01-CSAPP](../01-CSAPP-3rd/) · [03-SysPerf](../03-Systems-Performance-2nd/) 同一套约定。

---

## Part I · 技术（Ch 1–5）

| 章 | 导读 |
|----|------|
| 1 简介 | [chapter-01-introduction](./chapter-01-introduction/) |
| 2 技术背景 | [chapter-02-technology-background](./chapter-02-technology-background/) |
| 3 性能分析 | [chapter-03-performance-analysis](./chapter-03-performance-analysis/) |
| 4 BCC | [chapter-04-bcc](./chapter-04-bcc/) |
| 5 bpftrace | [chapter-05-bpftrace](./chapter-05-bpftrace/) |

## Part II · 使用 BPF 工具（Ch 6–16）

| 章 | 导读 |
|----|------|
| 6 CPU | [chapter-06-cpus](./chapter-06-cpus/) |
| 7 内存 | [chapter-07-memory](./chapter-07-memory/) |
| 8 文件系统 | [chapter-08-file-systems](./chapter-08-file-systems/) |
| 9 磁盘 I/O | [chapter-09-disk-io](./chapter-09-disk-io/) |
| 10 网络 | [chapter-10-networking](./chapter-10-networking/) |
| 11 安全 | [chapter-11-security](./chapter-11-security/) |
| 12 语言 | [chapter-12-languages](./chapter-12-languages/) |
| 13 应用程序 | [chapter-13-applications](./chapter-13-applications/) |
| 14 内核 | [chapter-14-kernel](./chapter-14-kernel/) |
| 15 容器 | [chapter-15-containers](./chapter-15-containers/) |
| 16 虚拟机管理程序 | [chapter-16-hypervisors](./chapter-16-hypervisors/) |

### HFT 延伸

| | 笔记 |
|---|------|
| XDP / tc-BPF | [note-XDP与tc-BPF.md](./note-XDP与tc-BPF.md) |

## Part III · 其他主题（Ch 17–18）

| 章 | 导读 |
|----|------|
| 17 其他 BPF 工具 | [chapter-17-other-tools](./chapter-17-other-tools/) |
| 18 技巧与常见问题 | [chapter-18-tips-and-tricks](./chapter-18-tips-and-tricks/) |

## Part IV · 附录（A–E）

| | 笔记 |
|---|------|
| A bpftrace 单行命令 | [appendix-A-bpftrace单行命令.md](./appendix-A-bpftrace单行命令.md) |
| B bpftrace 备忘单 | [appendix-B-bpftrace备忘单.md](./appendix-B-bpftrace备忘单.md) |
| C BCC 工具开发 | [appendix-C-BCC工具开发.md](./appendix-C-BCC工具开发.md) |
| D C 语言 BPF | [appendix-D-C语言BPF.md](./appendix-D-C语言BPF.md) |
| E BPF 指令 | [appendix-E-BPF指令.md](./appendix-E-BPF指令.md) |

---

## HFT 精读捷径

```
Ch 1–2 → Ch 4–5 → Ch 6 → Ch 10 (+ XDP note) → 附录 A/B
```

**HFT 产出：** 生产 eBPF 观测；与 DPDK 配合做内核栈 vs 用户态旁路对比。

## 交叉阅读

- **上一本（必读前置）** → [03-Systems-Performance-2nd](../03-Systems-Performance-2nd/) — 读完立刻读本目录
- SysPerf bpftrace 附录 → [02 appendix-C](../03-Systems-Performance-2nd/appendix-C-bpftrace单行命令.md)
- 后续内核/内存/网络 → [05-LKD](../05-Linux-Kernel-Development/) · [07-Gorman](../07-Linux-Virtual-Memory-Manager/) · [13-Rosen](../13-Linux-Kernel-Networking/)（读时可回头用 BPF 验证）
- DPDK 对照 → [14-DPDK](../14-DPDK-Low-Latency-Network/)
- 跨模块 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
