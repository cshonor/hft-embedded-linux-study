# UNP Vol.1 — Unix Network Programming（外部仓库）

**定位：** 用户态 Socket API · 与 [Linux Kernel Networking](../Linux-Kernel-Networking/)（内核）和 [TCP-IP-Illustrated-Vol1](../TCP-IP-Illustrated-Vol1/)（协议）组成网络三层。

**阅读顺序：** 第 **外B** 册 · 建议在 **TCP/IP 卷一之后、Rosen 之前或并行**

## 你的笔记仓库

<!-- 填入另一个仓库的链接，例如： -->
<!-- **笔记地址：** https://github.com/cshonor/your-network-notes/tree/main/UNP-Vol1 -->

**笔记地址：** _（待填入）_

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

- 协议层 → [TCP-IP-Illustrated-Vol1](../TCP-IP-Illustrated-Vol1/)
- 内核层 → [Linux-Kernel-Networking](../Linux-Kernel-Networking/)
- 程序员实践 → [CSAPP-3rd/chapter-08-网络编程.md](../CSAPP-3rd/chapter-08-网络编程.md)
