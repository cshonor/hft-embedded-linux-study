# UNP Vol.1 — Unix Network Programming（外部仓库）

**定位：** 用户态 Socket API · **标准内核网络路径**。与 04（协议）、06（内核实现）、12（DPDK 旁路）组成网络全链路。

**文件夹 05 · 外部书目 外B** · [返回总清单](../READING-LIST.md#外部书目笔记在另一仓库--本仓库仅索引)

## 笔记仓库（外部）

**仓库：** [cshonor/Computer-Networking](https://github.com/cshonor/Computer-Networking)

| 入口 | 链接 |
|------|------|
| UNP 卷1 笔记目录 | [UNP_Vol1/](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) |
| 阅读说明 | [README.md](https://github.com/cshonor/Computer-Networking/blob/main/UNP_Vol1/README.md) |
| 章节目录 | [OUTLINE.md](https://github.com/cshonor/Computer-Networking/blob/main/UNP_Vol1/OUTLINE.md) |

## HFT 必读 / 选读 / 跳过

| 原书章节 | 标签 | HFT 关联 |
|----------|------|----------|
| Ch 3 Socket 简介 | 🔴 必读 | `sockaddr`、字节序、基本 API |
| Ch 6 I/O 多路复用（select/poll/epoll） | 🔴 必读 | 单线程收多路行情 |
| Ch 7 Socket 选项 | 🔴 必读 | `TCP_NODELAY`、buffer、`SO_REUSEPORT` |
| Ch 8 UDP sockets | 🔴 必读 | 组播行情 `recvfrom` |
| Ch 16 非阻塞 I/O | 🔴 必读 | busy-poll / 自旋收包前置 |
| Ch 4–5 TCP/UDP 概述 | 🟡 选读 | 与 TCP/IP 卷一对照 |
| Ch 9–10 TCP 客户端/服务端 | 🟡 选读 | 订单走 TCP 时升为必读 |
| Ch 11 名字与时间 | 🟡 选读 | `getaddrinfo` |
| SCTP、RPC、复杂服务器 | ⚪ 跳过 | HFT 不用 |

## 为何不在本仓库展开

笔记已在你的网络书仓库维护；本仓库 [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md) 统一调度阅读顺序，避免重复维护。

## 交叉阅读

- 协议层 → [07-TCP-IP-Illustrated-Vol1](../07-TCP-IP-Illustrated-Vol1/)
- 内核层 → [09-Linux-Kernel-Networking](../09-Linux-Kernel-Networking/)
- 用户态旁路 → [10-DPDK-Low-Latency-Network](../10-DPDK-Low-Latency-Network/)
- 程序员实践 → [01-CSAPP-3rd/chapter-11-网络编程.md](../01-CSAPP-3rd/chapter-11-网络编程.md)
- 跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md#三dpdk--unp-socket-模型)
