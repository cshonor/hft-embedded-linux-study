# UNP Vol.1 — Unix Network Programming（外部仓库）

**定位：** 用户态 Socket API — 在 [09 PNP](../08-Practical-Network-Programming/) 与 **`08` TLPI** 之后，系统化 Stevens API。

**文件夹 `10`** · 外部书目 **外B**

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
| Ch 6 I/O 多路复用（select/poll/epoll） | 🔴 必读 | 对照 [PNP epoll 实验](https://github.com/cshonor/Computer-Networking/tree/main/PNP) |
| Ch 7 Socket 选项 | 🔴 必读 | `TCP_NODELAY`、buffer、`SO_REUSEPORT` |
| Ch 8 UDP sockets | 🔴 必读 | 组播行情 `recvfrom` |
| Ch 16 非阻塞 I/O | 🔴 必读 | busy-poll / 自旋收包前置 |
| Ch 4–5 TCP/UDP 概述 | 🟡 选读 | 与 [10 TCP/IP](../10-TCP-IP-Illustrated-Vol1/) 对照 |
| Ch 9–10 TCP 客户端/服务端 | 🟡 选读 | 订单走 TCP 时升为必读 |
| Ch 11 名字与时间 | 🟡 选读 | `getaddrinfo` |
| SCTP、RPC、复杂服务器 | ⚪ 跳过 | HFT 不用 |

## 为何不在本仓库展开

笔记已在你的网络书仓库维护；建议 **先 [10 PNP](../08-Practical-Network-Programming/) 再读本目录**。

## 交叉阅读

- **上一步 · TLPI：** [06-The-Linux-Programming-Interface](../06-The-Linux-Programming-Interface/)
- **上一步 · 实战：** [08-Practical-Network-Programming](../08-Practical-Network-Programming/)
- 协议层 → [10-TCP-IP-Illustrated-Vol1](../10-TCP-IP-Illustrated-Vol1/)
- 内核层 → [11-Linux-Kernel-Networking](../11-Linux-Kernel-Networking/)
- 用户态旁路 → [12-DPDK-Low-Latency-Network](../12-DPDK-Low-Latency-Network/)
- 程序员实践 → [01-CSAPP Ch11](../01-CSAPP-3rd/chapter-11-network-programming/)
- 跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
