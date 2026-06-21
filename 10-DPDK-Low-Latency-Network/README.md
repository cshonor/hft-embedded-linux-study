# DPDK Low-Latency Network — 用户态旁路网络

**文件夹 10** · 网络栈闭环 · [返回总清单](../READING-LIST.md#10-dpdk-用户态旁路网络官方文档--本仓库笔记)

> **定位：** 用户态轮询、PMD、mbuf、零拷贝旁路 — 与 `07`/`08`/`09` 内核网络路线**并行互补**。

## 网络全链路（07 → 08 → 09 → 10）

| 序号 | 文件夹 | 层级 | 回答的问题 |
|------|--------|------|-----------|
| 07 | [TCP/IP Illustrated Vol.1](../07-TCP-IP-Illustrated-Vol1/) | 协议 | 线上包长什么样？ |
| 08 | [UNP Vol.1](../08-UNP-Vol1/) | 系统调用 / Socket API | 用户态怎么调内核网络栈？ |
| 09 | [Linux Kernel Networking](../09-Linux-Kernel-Networking/) | 内核实现 | 内核怎么收发包？ |
| **10** | **本文件夹** | **用户态旁路** | **如何绕过内核栈、轮询收包？** |

两条路线对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md#二内核网络栈-vs-用户态旁路)

## 笔记目录

| 主题 | 笔记 | HFT |
|------|------|-----|
| 架构与 EAL | [chapter-01-DPDK架构与EAL.md](./chapter-01-DPDK架构与EAL.md) | 🔴 |
| mbuf 与内存池 | [chapter-02-mbuf与内存池.md](./chapter-02-mbuf与内存池.md) | 🔴 |
| PMD 与轮询模式 | [chapter-03-PMD与轮询模式.md](./chapter-03-PMD与轮询模式.md) | 🔴 |
| 零拷贝与用户态旁路 | [chapter-04-零拷贝与用户态旁路.md](./chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 组播行情接入 | [chapter-05-组播行情接入.md](./chapter-05-组播行情接入.md) | 🔴 |
| **实体书递进** | [note-DPDK实体书递进.md](./note-DPDK实体书递进.md) | 🟡 |
| OpenOnload / RDMA 对比 | [note-openonload-rdma对比.md](./note-openonload-rdma对比.md) | 🟡 |

📋 完整主题清单 → [OUTLINE.md](./OUTLINE.md)

## 官方文档（主参考）

| 资料 | 链接 |
|------|------|
| Programmer's Guide | https://doc.dpdk.org/guides/prog_guide/ |
| Sample Applications | https://doc.dpdk.org/guides/sample_app_ug/ |
| API Reference | https://doc.dpdk.org/api/ |

> 本文件夹主线：**DPDK 官方文档** + chapter 笔记 + 最小工程。  
> **实体书（场景触发）：** [《深入浅出 DPDK》→《Linux 高性能网络详解》](./note-DPDK实体书递进.md) — 先建立旁路认知，再串 RDMA/XDP 选型。

## 动手实验

| 实验 | 路径 | 状态 |
|------|------|------|
| 组播行情最小工程 | [code/mcast-minimal/](./code/mcast-minimal/) | 待实现 |

## 交叉阅读

- 内核栈对照 → [09-Linux-Kernel-Networking](../09-Linux-Kernel-Networking/)
- Socket 模型对照 → [08-UNP-Vol1](../08-UNP-Vol1/)、[01-CSAPP-3rd Ch11](../01-CSAPP-3rd/chapter-11-网络编程.md)
- 缓存 / 内存布局 → [01-CSAPP-3rd Ch6](../01-CSAPP-3rd/chapter-06-存储器层次结构.md)、[04-Computer-Architecture-6th](../04-Computer-Architecture-6th/)
- 生产观测 → [03-BPF-Performance-Tools](../03-BPF-Performance-Tools/)
- **交易系统工程落地**（非网络技术）→ [11-HFT-Low-Latency-Practice](../11-HFT-Low-Latency-Practice/)
