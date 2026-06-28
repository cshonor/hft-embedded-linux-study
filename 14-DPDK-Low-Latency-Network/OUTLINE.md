# DPDK — 主题目录

> 按实体书梯度组织：**01-Intro-Book** → **02-Advanced-Book**

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

---

## 01-Intro-Book · 《深入浅出 DPDK》

| # | 主题 | 笔记 | HFT |
|---|------|------|-----|
| — | 实体书递进 | [note-DPDK实体书递进](./01-Intro-Book/notes/note-DPDK实体书递进.md) | 🟡 |
| 1 | 认识 DPDK | [chapter-01-dpdk-intro/](./01-Intro-Book/chapter-01-dpdk-intro/) · [stub](./01-Intro-Book/notes/chapter-01-DPDK架构与EAL.md) | 🔴 |
| 2 | Cache 与内存 | [chapter-02-cache-and-memory/](./01-Intro-Book/chapter-02-cache-and-memory/) · [stub](./01-Intro-Book/notes/chapter-02-Cache与内存.md) | 🔴 |
| 3 | 并行计算 | [chapter-03-parallel-computing/](./01-Intro-Book/chapter-03-parallel-computing/) · [stub](./01-Intro-Book/notes/chapter-03-并行计算.md) | 🔴 |
| 4 | 同步互斥机制 | [chapter-04-synchronization/](./01-Intro-Book/chapter-04-synchronization/) · [stub](./01-Intro-Book/notes/chapter-04-同步互斥机制.md) | 🔴 |
| 5 | 报文转发 | [chapter-05-packet-forwarding/](./01-Intro-Book/chapter-05-packet-forwarding/) · [stub](./01-Intro-Book/notes/chapter-05-报文转发.md) | 🔴 |
| 6 | PCIe 与包处理 I/O | [chapter-06-pcie-packet-io/](./01-Intro-Book/chapter-06-pcie-packet-io/) · [stub](./01-Intro-Book/notes/chapter-06-PCIe与包处理IO.md) | 🔴 |
| 7 | 网卡性能优化 | [chapter-07-nic-performance-optimization/](./01-Intro-Book/chapter-07-nic-performance-optimization/) · [stub](./01-Intro-Book/notes/chapter-07-网卡性能优化.md) | 🔴 |
| 8 | mbuf、mempool | [chapter-02-mbuf](./01-Intro-Book/notes/chapter-02-mbuf与内存池.md) | 🔴 |
| 9 | PMD、poll mode | [chapter-03-PMD](./01-Intro-Book/notes/chapter-03-PMD与轮询模式.md) | 🔴 |
| 10 | 零拷贝、旁路 | [chapter-04-零拷贝](./01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 11 | UDP 组播行情 | [chapter-05-组播](./01-Intro-Book/notes/chapter-05-组播行情接入.md) | 🔴 |
| 12 | 流分类与多队列 | [chapter-08-flow-classification-multiqueue/](./01-Intro-Book/chapter-08-flow-classification-multiqueue/) · [stub](./01-Intro-Book/notes/chapter-08-流分类与多队列.md) | 🔴 |
| 13 | 硬件加速与卸载 | [chapter-09-hardware-offload/](./01-Intro-Book/chapter-09-hardware-offload/) · [stub](./01-Intro-Book/notes/chapter-09-硬件加速与功能卸载.md) | 🔴 |
| 14 | X86 I/O 虚拟化 | [chapter-10-x86-io-virtualization/](./01-Intro-Book/chapter-10-x86-io-virtualization/) · [stub](./01-Intro-Book/notes/chapter-10-X86平台IO虚拟化.md) | 🟡 |
| 15 | 半虚拟化 Virtio | [chapter-11-virtio-paravirtualization/](./01-Intro-Book/chapter-11-virtio-paravirtualization/) · [stub](./01-Intro-Book/notes/chapter-11-半虚拟化Virtio.md) | 🟡 |
| 16 | vhost 优化 | [chapter-12-vhost-optimization/](./01-Intro-Book/chapter-12-vhost-optimization/) · [stub](./01-Intro-Book/notes/chapter-12-vhost优化方案.md) | 🟡 |
| 17 | DPDK 与 NFV | [chapter-13-dpdk-nfv/](./01-Intro-Book/chapter-13-dpdk-nfv/) · [stub](./01-Intro-Book/notes/chapter-13-DPDK与网络功能虚拟化.md) | 🟡 |
| 18 | OVS 中的 DPDK 加速 | [chapter-14-ovs-dpdk-acceleration/](./01-Intro-Book/chapter-14-ovs-dpdk-acceleration/) · [stub](./01-Intro-Book/notes/chapter-14-Open-vSwitch中的DPDK性能加速.md) | 🟡 |

**code：** [01-Intro-Book/code/mcast-minimal/](./01-Intro-Book/code/mcast-minimal/)

---

## 02-Advanced-Book · 《Linux 高性能网络详解》

| 主题 | 笔记 | HFT |
|------|------|-----|
| OpenOnload / RDMA / RoCE | [note-openonload-rdma对比](./02-Advanced-Book/notes/note-openonload-rdma对比.md) | 🟡 |
| XDP / tc-BPF 对照 | [note-XDP与DPDK对照](./02-Advanced-Book/notes/note-XDP与DPDK对照.md) | 🟡 |

**code：** [02-Advanced-Book/code/](./02-Advanced-Book/code/)（待补充）

---

## 建议阅读顺序

```
09 Rosen
    ↓
01-Intro-Book：① 深入浅出 DPDK ∥ chapter-01–05 + 官方 doc
    ↓
02-Advanced-Book：② Linux 高性能网络详解 ∥ RDMA/XDP notes
    ↓
11 HFT Practice · ch06
```

> **何时开读：** `01`/`02` 打底 + perf 定位网络瓶颈。详见 [note-DPDK实体书递进](./01-Intro-Book/notes/note-DPDK实体书递进.md)。

跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
