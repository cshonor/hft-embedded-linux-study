# 01-Intro-Book · 《深入浅出 DPDK》

> **12-DPDK** 二级目录 · **梯度 ①** — 建立 DPDK 旁路认知  
> 实体书 + 官方 Programmer's Guide；与 [02-Advanced-Book](../02-Advanced-Book/) 递进。

---

## 目录结构

```
01-Intro-Book/
├── chapter-NN-english-slug/   ← 章节目录仅英文（便于 git）
│   └── notes/section-N-中文.md  ← 小节笔记文件名可中文
├── notes/     ← 章节 stub、读书导引
└── code/      ← 入门实验（组播最小工程等）
```

---

## notes · 章节笔记

| # | 主题 | 笔记 | HFT |
|---|------|------|-----|
| — | 实体书递进说明 | [note-DPDK实体书递进](./notes/note-DPDK实体书递进.md) | 🟡 |
| 1 | 认识 DPDK（实体书 Ch1） | [chapter-01-dpdk-intro/](./chapter-01-dpdk-intro/) · [stub](./notes/chapter-01-DPDK架构与EAL.md) | 🔴 |
| 2 | Cache 与内存（实体书 Ch2） | [chapter-02-cache-and-memory/](./chapter-02-cache-and-memory/) · [stub](./notes/chapter-02-Cache与内存.md) | 🔴 |
| 3 | 并行计算（实体书 Ch3） | [chapter-03-parallel-computing/](./chapter-03-parallel-computing/) · [stub](./notes/chapter-03-并行计算.md) | 🔴 |
| 4 | 同步互斥机制（实体书 Ch4） | [chapter-04-synchronization/](./chapter-04-synchronization/) · [stub](./notes/chapter-04-同步互斥机制.md) | 🔴 |
| 5 | 报文转发（实体书 Ch5） | [chapter-05-packet-forwarding/](./chapter-05-packet-forwarding/) · [stub](./notes/chapter-05-报文转发.md) | 🔴 |
| 6 | PCIe 与包处理 I/O（实体书 Ch6） | [chapter-06-pcie-packet-io/](./chapter-06-pcie-packet-io/) · [stub](./notes/chapter-06-PCIe与包处理IO.md) | 🔴 |
| 7 | 网卡性能优化（实体书 Ch7） | [chapter-07-nic-performance-optimization/](./chapter-07-nic-performance-optimization/) · [stub](./notes/chapter-07-网卡性能优化.md) | 🔴 |
| 8 | mbuf、mempool | [chapter-02-mbuf](./notes/chapter-02-mbuf与内存池.md) | 🔴 |
| 9 | PMD、轮询模式 | [chapter-03-PMD](./notes/chapter-03-PMD与轮询模式.md) | 🔴 |
| 10 | 零拷贝、旁路原理 | [chapter-04-零拷贝](./notes/chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 11 | UDP 组播行情 | [chapter-05-组播](./notes/chapter-05-组播行情接入.md) | 🔴 |
| 12 | 流分类与多队列（实体书 Ch8） | [chapter-08-flow-classification-multiqueue/](./chapter-08-flow-classification-multiqueue/) · [stub](./notes/chapter-08-流分类与多队列.md) | 🔴 |
| 13 | 硬件加速与卸载（实体书 Ch9） | [chapter-09-hardware-offload/](./chapter-09-hardware-offload/) · [stub](./notes/chapter-09-硬件加速与功能卸载.md) | 🔴 |
| 14 | X86 I/O 虚拟化（实体书 Ch10 · 虚拟化篇） | [chapter-10-x86-io-virtualization/](./chapter-10-x86-io-virtualization/) · [stub](./notes/chapter-10-X86平台IO虚拟化.md) | 🟡 |
| 15 | 半虚拟化 Virtio（实体书 Ch11） | [chapter-11-virtio-paravirtualization/](./chapter-11-virtio-paravirtualization/) · [stub](./notes/chapter-11-半虚拟化Virtio.md) | 🟡 |
| 16 | vhost 优化（实体书 Ch12） | [chapter-12-vhost-optimization/](./chapter-12-vhost-optimization/) · [stub](./notes/chapter-12-vhost优化方案.md) | 🟡 |
| 17 | DPDK 与 NFV（实体书 Ch13 · 应用篇） | [chapter-13-dpdk-nfv/](./chapter-13-dpdk-nfv/) · [stub](./notes/chapter-13-DPDK与网络功能虚拟化.md) | 🟡 |
| 18 | OVS 中的 DPDK 加速（实体书 Ch14） | [chapter-14-ovs-dpdk-acceleration/](./chapter-14-ovs-dpdk-acceleration/) · [stub](./notes/chapter-14-Open-vSwitch中的DPDK性能加速.md) | 🟡 |
| 19 | 基于 DPDK 的存储优化（实体书 Ch15 · 应用篇压轴） | [chapter-15-dpdk-storage-optimization/](./chapter-15-dpdk-storage-optimization/) · [stub](./notes/chapter-15-基于DPDK的存储软件优化.md) | 🟡 |

---

## code · 实验

| 实验 | 路径 | 状态 |
|------|------|------|
| 组播行情最小工程 | [mcast-minimal/](./code/mcast-minimal/) | 待实现 |

---

## 读完之后

→ [02-Advanced-Book](../02-Advanced-Book/) · 《Linux 高性能网络详解》（RDMA / XDP / 选型）  
→ [10 总目录](../README.md)
