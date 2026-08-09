# Ch 2 Netlink 套接字 · Netlink Sockets

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

> 本章详解 **Netlink** 的实现与 API — Linux **内核 ↔ 用户空间**、以及 **内核子系统之间** 的 **双向 IPC** 核心机制。现代 **`iproute2`**（`ip`、`ss`）全建立在 Netlink 上；老 **`net-tools`**（`ifconfig`）走 **ioctl**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | Netlink 相对 ioctl 的优势 · 创建/交互 · `nlmsghdr`/TLV · Generic Netlink · `sock_diag` |
| **前置** | [Ch 1](../chapter-01-introduction/) — 网络栈与 `net_device` 概念 |
| **HFT 读法** | 交易路径 **不经过 Netlink**；但 **绑核、多队列、路由、排查 `ss`** 都依赖它 — **运维/调机选读** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. Netlink 基础与优势 | [notes/section-1-Netlink基础与优势.md](./notes/section-1-Netlink基础与优势.md) |
| 2. 套接字的创建与交互 | [notes/section-2-套接字的创建与交互.md](./notes/section-2-套接字的创建与交互.md) |
| 3. 数据结构与消息格式 | [notes/section-3-数据结构与消息格式.md](./notes/section-3-数据结构与消息格式.md) |
| 4. 通用 Netlink 协议 | [notes/section-4-通用Netlink协议.md](./notes/section-4-通用Netlink协议.md) |
| 5. 套接字监控 `sock_diag` | [notes/section-5-套接字监控-sock_diag.md](./notes/section-5-套接字监控-sock_diag.md) |

---

## 相关章节

- 上一章：[../chapter-01-introduction/](../chapter-01-introduction/)
- 下一章：[../chapter-03-icmp/](../chapter-03-icmp/)
- 附录 B 网络管理：[../appendix-B-网络管理.md](../appendix-B-网络管理.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
