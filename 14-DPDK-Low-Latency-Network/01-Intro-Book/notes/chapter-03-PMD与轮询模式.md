# Ch 3 PMD 与轮询模式 · PMD & Poll Mode

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **实体书：** [chapter-07-nic-performance-optimization](./chapter-07-nic-performance-optimization/) 精讲轮询/混合中断、Burst、平台调优；[chapter-06-pcie-packet-io](./chapter-06-pcie-packet-io/) 描述符环与 MMIO。

<!-- 实验向笔记待补充：Poll Mode Driver、rx/tx burst、队列与 lcore 绑定 -->

## 相关章节

- 实体书：[chapter-07-nic-performance-optimization/](./chapter-07-nic-performance-optimization/) · [chapter-06-pcie-packet-io/](./chapter-06-pcie-packet-io/)
- 上一章：[chapter-02-Cache与内存.md](./chapter-02-Cache与内存.md) · [chapter-02-mbuf与内存池.md](./chapter-02-mbuf与内存池.md)
- 下一章：[chapter-04-零拷贝与用户态旁路.md](./chapter-04-零拷贝与用户态旁路.md)
- 内核对照：[13-LKN Ch14](../../../13-Linux-Kernel-Networking/chapter-14-advanced-topics/) · [chapter-08 流分类与多队列](./chapter-08-流分类与多队列.md)
