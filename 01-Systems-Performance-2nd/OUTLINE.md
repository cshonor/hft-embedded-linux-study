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
| 1 | Introduction | [chapter-01](./chapter-01-简介.md) | 🔴 |
| 2 | Methodologies | [chapter-02](./chapter-02-方法论.md) | 🔴 |
| 3 | Operating Systems | [chapter-03](./chapter-03-操作系统.md) | 🟡 |
| 4 | Observability Tools | [chapter-04](./chapter-04-观测工具.md) | 🔴 |
| 5 | Applications | [chapter-05](./chapter-05-应用程序.md) | 🟡 |
| 6 | CPUs | [chapter-06](./chapter-06-中央处理器.md) | 🔴 |
| 7 | Memory | [chapter-07](./chapter-07-内存.md) | 🔴 |
| 8 | File Systems | [chapter-08](./chapter-08-文件系统.md) | ⚪ |
| 9 | Disks | [chapter-09](./chapter-09-磁盘.md) | ⚪ |
| 10 | Network | [chapter-10](./chapter-10-网络.md) | 🔴 |
| 11 | Cloud Computing | [chapter-11](./chapter-11-云计算.md) | ⚪ |
| 12 | Benchmarking | [chapter-12](./chapter-12-基准测试.md) | 🟡 |
| 13 | perf | [chapter-13](./chapter-13-perf性能分析.md) | 🔴 |
| 14 | Ftrace | [chapter-14](./chapter-14-Ftrace跟踪.md) | 🟡 |
| 15 | BPF | [chapter-15](./chapter-15-BPF技术.md) | 🔴 |
| 16 | Case Study | [chapter-16](./chapter-16-案例研究.md) | 🟡 |

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

```
Ch 1–2  方法论（USE/RED）
Ch 4    观测工具
Ch 6–7  CPU / 内存
Ch 10   网络（含 TCP/UDP 协议栈）
Ch 13   perf
Ch 15   BPF
附录 A/C
```

→ 深入 BPF → [09-BPF-Performance-Tools](../09-BPF-Performance-Tools/)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
