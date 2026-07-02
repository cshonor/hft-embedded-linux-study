# Ch 11 第 4 层协议 · Layer 4 Protocols

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

> **L4** — **Socket API**、`struct socket` / `struct sock`、**UDP/TCP** 收发路径；辅以 **SCTP/DCCP**。HFT **行情多 UDP 组播、发单多 TCP**；本章是 **内核栈 → 用户态** 的 **核心桥梁**，与 [Ch 14](../chapter-14-advanced-topics/) **NAPI/softirq** 联读。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | Socket · UDP · TCP · SCTP · DCCP |
| **前置** | [Ch 4 IPv4](../chapter-04-ipv4/) · [Ch 1 §4 包流转](../chapter-01-introduction/notes/section-4-数据包的收发与流转.md) |
| **HFT 读法** | **§1–§3 必精读**；SCTP/DCCP 知即可；对照 [UNP](../../09-UNP-Vol1/) · [DPDK 旁路 UDP/TCP](../../12-DPDK-Low-Latency-Network/) |

---

## 小节笔记

| 节 | 笔记 | HFT |
|----|------|-----|
| 1. 套接字 Sockets | [notes/section-1-套接字.md](./notes/section-1-套接字.md) | 🔴 |
| 2. UDP | [notes/section-2-UDP.md](./notes/section-2-UDP.md) | 🔴 |
| 3. TCP | [notes/section-3-TCP.md](./notes/section-3-TCP.md) | 🔴 |
| 4. SCTP | [notes/section-4-SCTP.md](./notes/section-4-SCTP.md) | 🟡 |
| 5. DCCP | [notes/section-5-DCCP.md](./notes/section-5-DCCP.md) | ⚪ |

---

## 相关章节

- 上一章：[../chapter-10-ipsec/](../chapter-10-ipsec/)
- 下一章：[../chapter-12-wireless-in-linux/](../chapter-12-wireless-in-linux/)
- 组播延伸：[../note-组播IGMP.md](../note-组播IGMP.md)
- 高级主题 NAPI/RPS：[../chapter-14-advanced-topics/](../chapter-14-advanced-topics/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
