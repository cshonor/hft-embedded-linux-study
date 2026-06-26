# XDP / tc-BPF 与 DPDK 对照

> **02-Advanced-Book** · 《Linux 高性能网络详解》配套 · **选读**

<!-- 章节笔记待补充：与实体书 XDP 章节对照 -->

## 与 DPDK 的分工

| | DPDK（01-Intro） | XDP / tc-BPF |
|---|------------------|--------------|
| 位置 | 用户态完全旁路 | 内核最早 hook 点 |
| 典型用途 | UDP 组播行情、极致 poll | 早期过滤、统计、部分 redirect |
| 开发成本 | 高（无 socket 语义） | 中（仍在内核 eBPF 生态） |

→ 深入 XDP 实现与工具：[04-BPF note-XDP与tc-BPF](../../../04-BPF-Performance-Tools/note-XDP与tc-BPF.md)  
→ 方案总表：[note-openonload-rdma对比](./note-openonload-rdma对比.md)

## 相关章节

- 上一梯度：[01-Intro-Book](../../01-Intro-Book/)
- [10 总目录](../../README.md)
