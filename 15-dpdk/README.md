# DPDK Low-Latency Network — 用户态旁路网络

**文件夹 15** · 网络栈闭环 · [返回总清单](../READING-LIST.md#10-dpdk-用户态旁路网络官方文档--本仓库笔记)

> **定位：** 用户态轮询、PMD、mbuf、零拷贝旁路 — 与 `05`/`17`/`06` 内核网络路线**并行互补**。

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
| 07 | [TCP/IP Illustrated Vol.1](../13-tcpip-protocols/) | 协议 | 线上包长什么样？ |
| 08 | [UNP Vol.1](../12-network-sockets/01-unix-network-api/) | 系统调用 / Socket API | 用户态怎么调内核网络栈？ |
| 09 | [Linux Kernel Networking](../14-kernel-networking/) | 内核实现 | 内核怎么收发包？ |
| **08** | **本文件夹** | **用户态旁路** | **如何绕过内核栈、轮询收包？** |

两条路线对照 → [README.md](../README.md)

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

- 内核栈对照 → [14-kernel-networking](../14-kernel-networking/)
- Socket 模型 → [12-network-sockets/01-unix-network-api](../12-network-sockets/01-unix-network-api/)、[02-computer-systems Ch11](../02-computer-systems/chapter-11-network-programming/)
- 缓存 / 内存 → [02-computer-systems Ch6](../02-computer-systems/chapter-06-memory-hierarchy/)、[19-computer-architecture](../19-computer-architecture/)
- 生产观测 → [17-bpf-observability](../17-bpf-observability/)
- 工程落地 → [18-hft-engineering](../18-hft-engineering/)
