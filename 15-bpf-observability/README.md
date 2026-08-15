# BPF Performance Tools — Brendan Gregg

**文件夹 15** · 全书 **18 章 + 附录 A–E**（4 Parts）· [返回总清单](../READING-LIST.md#8-bpf-performance-tools--brendan-gregg)

> **文件夹 15** · 紧接 [14-Systems-Performance](../14-systems-performance/) · **同样后置**（见 [README](../README.md)）。  
> **建议时机：** 已有 Linux 内核/网络/DPDK 或 HFT 压测靶子后再开 — 用 BPF 验证真实系统。  
> **后续：** [18-HFT](../16-hft-engineering/) / [21-Rust](../20-rust-quant/)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 目录结构

```
chapter-XX-english-slug/
├── README.md      ← 章导读（小节索引、大白话、Checklist）
└── notes/         ← 按原书小节拆分的笔记
```

与 [02-CSAPP](../02-computer-systems/) · [14-Systems-Performance](../14-systems-performance/) 同一套约定。

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

## 📚 eBPF O'Reilly 三书对比

本模块主书为《BPF Performance Tools》（Gregg）。另外两本 O'Reilly eBPF 书的对比与协同读法见 [EBPF-BOOKS-COMPARISON.md](./EBPF-BOOKS-COMPARISON.md)（Learning eBPF 现代入门首选 → 本模块工具谱系 → Linux Observability 观测案例补充）。

### 书籍笔记

| 书 | 笔记 | 定位 |
|----|------|------|
| BPF Performance Tools（本模块主书） | [BPF-PERFORMANCE-TOOLS-EVAL.md](./BPF-PERFORMANCE-TOOLS-EVAL.md) | 工具谱系百科，按 OUTLINE 🔴 精读 |
| Learning eBPF（Liz Rice） | [LEARNING-EBPF-NOTES.md](./LEARNING-EBPF-NOTES.md) | 现代原理补充，libbpf/CO-RE/验证器/程序类型 |

## 19↔20 联动参考文档

| 文档 | 用途 |
|------|------|
| [Rubric 评审校验清单](./ref-rubric-checklist.md) | 一次排障/优化做完后逐项打勾（6 大类 30+ 检查项） |
| [bpftrace 样例脚本集](./ref-bpftrace-scripts.md) | 8 个场景脚本：调度/IO/TCP/syscall/锁/缺页/slab/软中断 |
| [故障排查决策树](./ref-troubleshooting-decision-tree.md) | 7 大症状入口 → 19 看现象 → 20 钻根因 |

## 交叉阅读

- **上一本（必读前置）** → [14-systems-performance](../14-systems-performance/) — 读完立刻读本目录
- SysPerf bpftrace 附录 → [02 appendix-C](../14-systems-performance/appendix-C-bpftrace单行命令.md)
- 后续内核/内存/网络 → [05-linux-kernel](../05-linux-kernel/) · [06-linux-mm](../06-linux-mm/) · [14-Rosen](../12-kernel-networking/)（读时可回头用 BPF 验证）
- DPDK 对照 → [15-DPDK](../13-dpdk/)
- 跨模块 → [README.md](../README.md)
