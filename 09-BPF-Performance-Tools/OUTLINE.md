# BPF Performance Tools — 全书目录（18 章 + 附录 A–E）

> **BPF Performance Tools** · Brendan Gregg

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## Part I · 技术 (Technologies)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01](./chapter-01-简介.md) | 🔴 |
| 2 | Technology Background | [chapter-02](./chapter-02-技术背景.md) | 🔴 |
| 3 | Performance Analysis | [chapter-03](./chapter-03-性能分析.md) | 🟡 |
| 4 | BCC | [chapter-04](./chapter-04-BCC.md) | 🔴 |
| 5 | bpftrace | [chapter-05](./chapter-05-bpftrace.md) | 🔴 |

## Part II · 使用 BPF 工具 (Using BPF Tools)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 6 | CPUs | [chapter-06](./chapter-06-CPU.md) | 🔴 |
| 7 | Memory | [chapter-07](./chapter-07-内存.md) | 🟡 |
| 8 | File Systems | [chapter-08](./chapter-08-文件系统.md) | ⚪ |
| 9 | Disk I/O | [chapter-09](./chapter-09-磁盘IO.md) | ⚪ |
| 10 | Networking | [chapter-10](./chapter-10-网络.md) | 🔴 |
| 11 | Security | [chapter-11](./chapter-11-安全.md) | ⚪ |
| 12 | Languages | [chapter-12](./chapter-12-语言.md) | ⚪ |
| 13 | Applications | [chapter-13](./chapter-13-应用程序.md) | 🟡 |
| 14 | Kernel | [chapter-14](./chapter-14-内核.md) | 🟡 |
| 15 | Containers | [chapter-15](./chapter-15-容器.md) | ⚪ |
| 16 | Hypervisors | [chapter-16](./chapter-16-虚拟机管理程序.md) | ⚪ |

### HFT 延伸

| 主题 | 笔记 | HFT |
|------|------|-----|
| XDP / tc-BPF | [note-XDP与tc-BPF](./note-XDP与tc-BPF.md) | 🔴 |

## Part III · 其他主题 (Additional Topics)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 17 | Other BPF Performance Tools | [chapter-17](./chapter-17-其他BPF工具.md) | 🟡 |
| 18 | Tips, Tricks, and Common Problems | [chapter-18](./chapter-18-技巧与常见问题.md) | 🟡 |

## Part IV · 附录 (Appendixes)

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | bpftrace One-Liners | [appendix-A](./appendix-A-bpftrace单行命令.md) | 🔴 |
| B | bpftrace Cheat Sheet | [appendix-B](./appendix-B-bpftrace备忘单.md) | 🔴 |
| C | BCC Tool Development | [appendix-C](./appendix-C-BCC工具开发.md) | 🟡 |
| D | C BPF | [appendix-D](./appendix-D-C语言BPF.md) | 🟡 |
| E | BPF Instructions | [appendix-E](./appendix-E-BPF指令.md) | ⚪ |

> 前言 / 词汇表 / 参考文献 / 索引：不单独建笔记文件。

---

## HFT 精读顺序

```
Part I   Ch 1–2 → Ch 4–5（BCC/bpftrace 上手）
Part II  Ch 6 CPU → Ch 10 网络 (+ note-XDP)
         Ch 7 内存（有余力）
附录 A/B bpftrace 速查
```

→ 衔接 [01-SysPerf](../01-Systems-Performance-2nd/) · vs DPDK → [12-DPDK](../12-DPDK-Low-Latency-Network/)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
