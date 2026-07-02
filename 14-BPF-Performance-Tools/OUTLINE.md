# BPF Performance Tools — 全书目录（18 章 + 附录 A–E）

> **BPF Performance Tools** · Brendan Gregg

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## Part I · 技术 (Technologies)

| 章 | 英文 | 导读 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01-introduction](./chapter-01-introduction/) | 🔴 |
| 2 | Technology Background | [chapter-02-technology-background](./chapter-02-technology-background/) | 🔴 |
| 3 | Performance Analysis | [chapter-03-performance-analysis](./chapter-03-performance-analysis/) | 🟡 |
| 4 | BCC | [chapter-04-bcc](./chapter-04-bcc/) | 🔴 |
| 5 | bpftrace | [chapter-05-bpftrace](./chapter-05-bpftrace/) | 🔴 |

## Part II · 使用 BPF 工具 (Using BPF Tools)

| 章 | 英文 | 导读 | HFT |
|----|------|------|-----|
| 6 | CPUs | [chapter-06-cpus](./chapter-06-cpus/) | 🔴 |
| 7 | Memory | [chapter-07-memory](./chapter-07-memory/) | 🟡 |
| 8 | File Systems | [chapter-08-file-systems](./chapter-08-file-systems/) | ⚪ |
| 9 | Disk I/O | [chapter-09-disk-io](./chapter-09-disk-io/) | ⚪ |
| 10 | Networking | [chapter-10-networking](./chapter-10-networking/) | 🔴 |
| 11 | Security | [chapter-11-security](./chapter-11-security/) | ⚪ |
| 12 | Languages | [chapter-12-languages](./chapter-12-languages/) | ⚪ |
| 13 | Applications | [chapter-13-applications](./chapter-13-applications/) | 🟡 |
| 14 | Kernel | [chapter-14-kernel](./chapter-14-kernel/) | 🟡 |
| 15 | Containers | [chapter-15-containers](./chapter-15-containers/) | ⚪ |
| 16 | Hypervisors | [chapter-16-hypervisors](./chapter-16-hypervisors/) | ⚪ |

### HFT 延伸

| 主题 | 笔记 | HFT |
|------|------|-----|
| XDP / tc-BPF | [note-XDP与tc-BPF](./note-XDP与tc-BPF.md) | 🔴 |

## Part III · 其他主题 (Additional Topics)

| 章 | 英文 | 导读 | HFT |
|----|------|------|-----|
| 17 | Other BPF Performance Tools | [chapter-17-other-tools](./chapter-17-other-tools/) | 🟡 |
| 18 | Tips, Tricks, and Common Problems | [chapter-18-tips-and-tricks](./chapter-18-tips-and-tricks/) | 🟡 |

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

→ **紧接** [13-Systems-Performance](../13-Systems-Performance-2nd/)（Gregg 双书第二本）· vs DPDK → [12-DPDK](../12-DPDK-Low-Latency-Network/)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
