# DPDK Low-Latency Network — 用户态旁路网络

**文件夹 10** · 网络栈闭环 · [返回总清单](../READING-LIST.md#10-dpdk-用户态旁路网络官方文档--本仓库笔记)

> **定位：** 用户态轮询、PMD、mbuf、零拷贝旁路 — 与 `07`/`08`/`09` 内核网络路线**并行互补**。

## 二级目录 · 按实体书梯度

| 目录 | 实体书 | 内容 |
|------|--------|------|
| **[01-Intro-Book](./01-Intro-Book/)** | 《深入浅出 DPDK》 | `notes/` 章节笔记 + `code/` 入门实验 |
| **[02-Advanced-Book](./02-Advanced-Book/)** | 《Linux 高性能网络详解》 | `notes/` RDMA/XDP 等 + `code/` 进阶实验 |

→ 两书递进说明：[01-Intro-Book/notes/note-DPDK实体书递进.md](./01-Intro-Book/notes/note-DPDK实体书递进.md)

---

## 网络全链路（08 → 09 → 10 → 11）

| 序号 | 文件夹 | 层级 | 回答的问题 |
|------|--------|------|-----------|
| 07 | [TCP/IP Illustrated Vol.1](../10-TCP-IP-Illustrated-Vol1/) | 协议 | 线上包长什么样？ |
| 08 | [UNP Vol.1](../09-UNP-Vol1/) | 系统调用 / Socket API | 用户态怎么调内核网络栈？ |
| 09 | [Linux Kernel Networking](../11-Linux-Kernel-Networking/) | 内核实现 | 内核怎么收发包？ |
| **08** | **本文件夹** | **用户态旁路** | **如何绕过内核栈、轮询收包？** |

两条路线对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md#二内核网络栈-vs-用户态旁路)

📋 完整主题清单 → [OUTLINE.md](./OUTLINE.md)

---

## 官方文档（01-Intro 主参考）

| 资料 | 链接 |
|------|------|
| Programmer's Guide | https://doc.dpdk.org/guides/prog_guide/ |
| Sample Applications | https://doc.dpdk.org/guides/sample_app_ug/ |
| API Reference | https://doc.dpdk.org/api/ |

---

## 交叉阅读

- 内核栈对照 → [11-Linux-Kernel-Networking](../11-Linux-Kernel-Networking/)
- Socket 模型 → [09-UNP-Vol1](../09-UNP-Vol1/)、[01-CSAPP-3rd Ch11](../01-CSAPP-3rd/chapter-11-network-programming/)
- 缓存 / 内存 → [01-CSAPP-3rd Ch6](../01-CSAPP-3rd/chapter-06-memory-hierarchy/)、[02-Computer-Architecture-6th](../02-Computer-Architecture-6th/)
- 生产观测 → [14-BPF-Performance-Tools](../14-BPF-Performance-Tools/)
- 工程落地 → [15-HFT-Low-Latency-Practice](../15-HFT-Low-Latency-Practice/)
