# BPF Performance Tools — Brendan Gregg

**文件夹 09** · 全书 **18 章 + 附录 A–E**（4 Parts）· [返回总清单](../READING-LIST.md#8-bpf-performance-tools--brendan-gregg)

> **紧接 [01-SysPerf](../01-Systems-Performance-2nd/) 阅读** — Gregg 性能双书第二本；SysPerf Ch 15 的 bpftrace/BCC 在此全书展开。

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## Part I · 技术（Ch 1–5）

| 章 | 笔记 |
|----|------|
| 1 简介 | [chapter-01-简介.md](./chapter-01-简介.md) |
| 2 技术背景 | [chapter-02-技术背景.md](./chapter-02-技术背景.md) |
| 3 性能分析 | [chapter-03-性能分析.md](./chapter-03-性能分析.md) |
| 4 BCC | [chapter-04-BCC.md](./chapter-04-BCC.md) |
| 5 bpftrace | [chapter-05-bpftrace.md](./chapter-05-bpftrace.md) |

## Part II · 使用 BPF 工具（Ch 6–16）

| 章 | 笔记 |
|----|------|
| 6 CPU | [chapter-06-CPU.md](./chapter-06-CPU.md) |
| 7 内存 | [chapter-07-内存.md](./chapter-07-内存.md) |
| 8 文件系统 | [chapter-08-文件系统.md](./chapter-08-文件系统.md) |
| 9 磁盘 I/O | [chapter-09-磁盘IO.md](./chapter-09-磁盘IO.md) |
| 10 网络 | [chapter-10-网络.md](./chapter-10-网络.md) |
| 11 安全 | [chapter-11-安全.md](./chapter-11-安全.md) |
| 12 语言 | [chapter-12-语言.md](./chapter-12-语言.md) |
| 13 应用程序 | [chapter-13-应用程序.md](./chapter-13-应用程序.md) |
| 14 内核 | [chapter-14-内核.md](./chapter-14-内核.md) |
| 15 容器 | [chapter-15-容器.md](./chapter-15-容器.md) |
| 16 虚拟机管理程序 | [chapter-16-虚拟机管理程序.md](./chapter-16-虚拟机管理程序.md) |

### HFT 延伸

| | 笔记 |
|---|------|
| XDP / tc-BPF | [note-XDP与tc-BPF.md](./note-XDP与tc-BPF.md) |

## Part III · 其他主题（Ch 17–18）

| 章 | 笔记 |
|----|------|
| 17 其他 BPF 工具 | [chapter-17-其他BPF工具.md](./chapter-17-其他BPF工具.md) |
| 18 技巧与常见问题 | [chapter-18-技巧与常见问题.md](./chapter-18-技巧与常见问题.md) |

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

- **上一本（必读前置）** → [01-Systems-Performance-2nd](../01-Systems-Performance-2nd/) — 读完立刻读本目录
- SysPerf bpftrace 附录 → [01 appendix-C](../01-Systems-Performance-2nd/appendix-C-bpftrace单行命令.md)
- 后续内核/内存/网络 → [02-LKD](../02-Linux-Kernel-Development/) · [03-Gorman](../03-Linux-Virtual-Memory-Manager/) · [06-Rosen](../06-Linux-Kernel-Networking/)（读时可回头用 BPF 验证）
- DPDK 对照 → [12-DPDK](../12-DPDK-Low-Latency-Network/)
- 跨模块 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
