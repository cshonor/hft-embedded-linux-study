# Systems Performance 2nd — Brendan Gregg

**文件夹 02** · 全书 **16 章 + 附录 A–E** · [返回总清单](../READING-LIST.md#1-systems-performance-enterprise-and-the-cloud-2nd--brendan-gregg)

> **文件夹 02** · 知其然 — 性能分析方法论。  
> **前置：** [01-CSAPP](../01-CSAPP-3rd/) · **下一本：** [04-BPF](../04-BPF-Performance-Tools/)（紧接）  
> 全链路 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 目录结构

```
chapter-XX-english-slug/
├── README.md      ← 章导读（中文标题、Checklist、HFT 捷径）
└── notes/         ← 按原书小节拆分的笔记（可增量追加）
```

每章 `README.md` 含 **小节笔记索引表**；新增内容写入 `notes/section-*.md`，不必再堆单文件。

---

## 核心章节（16 章）

| 章 | 导读 |
|----|------|
| 1 简介 | [chapter-01-intro](./chapter-01-intro/) |
| 2 方法论 | [chapter-02-methodologies](./chapter-02-methodologies/) |
| 3 操作系统 | [chapter-03-operating-systems](./chapter-03-operating-systems/) |
| 4 观测工具 | [chapter-04-observability-tools](./chapter-04-observability-tools/) |
| 5 应用程序 | [chapter-05-applications](./chapter-05-applications/) |
| 6 中央处理器 | [chapter-06-cpus](./chapter-06-cpus/) |
| 7 内存 | [chapter-07-memory](./chapter-07-memory/) |
| 8 文件系统 | [chapter-08-file-systems](./chapter-08-file-systems/) |
| 9 磁盘 | [chapter-09-disks](./chapter-09-disks/) |
| 10 网络 | [chapter-10-network](./chapter-10-network/) |
| 11 云计算 | [chapter-11-cloud-computing](./chapter-11-cloud-computing/) |
| 12 基准测试 | [chapter-12-benchmarking](./chapter-12-benchmarking/) |
| 13 perf | [chapter-13-perf](./chapter-13-perf/) |
| 14 Ftrace | [chapter-14-ftrace](./chapter-14-ftrace/) |
| 15 BPF | [chapter-15-bpf](./chapter-15-bpf/) |
| 16 案例研究 | [chapter-16-case-studies](./chapter-16-case-studies/) |

## 附录

| | 笔记 |
|---|------|
| A USE 方法 (Linux) | [appendix-A-USE方法Linux.md](./appendix-A-USE方法Linux.md) |
| B sar 总结 | [appendix-B-sar总结.md](./appendix-B-sar总结.md) |
| C bpftrace 单行命令 | [appendix-C-bpftrace单行命令.md](./appendix-C-bpftrace单行命令.md) |
| D 习题解答 | [appendix-D-习题解答.md](./appendix-D-习题解答.md) |
| E 性能领域人物 | [appendix-E-性能领域人物.md](./appendix-E-性能领域人物.md) |

---

## HFT 精读捷径

```
Ch 1–2 → Ch 4 → Ch 6–7 → Ch 10 → Ch 13 → Ch 15 → 附录 A/C
```

**下一本（紧接）：** [04-BPF-Performance-Tools](../04-BPF-Performance-Tools/) — Ch 15 预览在此展开为 bpftrace/BCC 全书。
