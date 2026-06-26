# SysPerf 2nd — 全书目录（16 章 + 附录 A–E）

> **Systems Performance: Enterprise and the Cloud 2nd** · Brendan Gregg

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## 核心章节

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01](./chapter-01-intro/) | 🔴 |
| 2 | Methodologies | [chapter-02](./chapter-02-methodologies/) | 🔴 |
| 3 | Operating Systems | [chapter-03](./chapter-03-operating-systems/) | 🟡 |
| 4 | Observability Tools | [chapter-04](./chapter-04-observability-tools/) | 🔴 |
| 5 | Applications | [chapter-05](./chapter-05-applications/) | 🔴 |
| 6 | CPUs | [chapter-06](./chapter-06-cpus/) | 🔴 |
| 7 | Memory | [chapter-07](./chapter-07-memory/) | 🔴 |
| 8 | File Systems | [chapter-08](./chapter-08-file-systems/) | 🟡 |
| 9 | Disks | [chapter-09](./chapter-09-disks/) | 🟡 |
| 10 | Network | [chapter-10](./chapter-10-network/) | 🔴 |
| 11 | Cloud Computing | [chapter-11](./chapter-11-cloud-computing/) | ⚪ |
| 12 | Benchmarking | [chapter-12](./chapter-12-benchmarking/) | 🟡 |
| 13 | perf | [chapter-13](./chapter-13-perf/) | 🔴 |
| 16 | Ftrace | [chapter-14](./chapter-14-ftrace/) | 🟡 |
| 16 | BPF | [chapter-15](./chapter-15-bpf/) | 🔴 |
| 16 | Case Study | [chapter-16](./chapter-16-case-studies/) | 🟡 |

## 附录

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | USE Method: Linux | [appendix-A](./appendix-A-USE方法Linux.md) | 🔴 |
| B | sar Summary | [appendix-B](./appendix-B-sar总结.md) | 🟡 |
| C | bpftrace One-Liners | [appendix-C](./appendix-C-bpftrace单行命令.md) | 🔴 |
| D | Solutions to Exercises | [appendix-D](./appendix-D-习题解答.md) | ⚪ |
| E | Who's Who | [appendix-E](./appendix-E-性能领域人物.md) | ⚪ |

> 前言 / 致谢 / 术语表 / 索引：不单独建笔记文件。

---

## HFT 精读顺序

> **全书前置（阶段 1）：** [01-CSAPP](../01-CSAPP-3rd/) 地基 + [02-Hennessy](../02-Computer-Architecture-6th/) Ch2 — 再读本目录。

```
Ch 1–2  方法论（USE/RED）
Ch 4    观测工具
Ch 6–7  CPU / 内存
Ch 10   网络（含 TCP/UDP 协议栈）
Ch 13   perf
Ch 15   BPF
附录 A/C
```

→ 深入 BPF → [04-BPF-Performance-Tools](../04-BPF-Performance-Tools/)（**紧接本书**，不必等后续章节）

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
