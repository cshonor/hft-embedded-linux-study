# Ch 3 ICMP · Internet Control Message Protocol

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> **ICMP** 是 **L4 协议**，向 L3 提供 **差错报告** 与 **诊断查询** — `ping` / `traceroute` 的底层机制，也是栈内 **不可达、PMTU、超时** 等异常的 **反馈通道**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | ICMPv4 收发与限速 · ICMPv6/ND/MLD · Ping 套接字 · iptables REJECT |
| **前置** | [Ch 1](../chapter-01-introduction/) 栈流转 · [Ch 4](../chapter-04-ipv4/) IPv4 主体（可并行） |
| **HFT 读法** | **PMTU / 不可达** 影响连接建立；**ICMP rate limit** 与防火墙 **REJECT** 影响排查 — 非 tick 热路径但 **连通性/运维** 相关 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. ICMPv4 的实现与消息流转 | [notes/section-1-ICMPv4的实现与消息流转.md](./notes/section-1-ICMPv4的实现与消息流转.md) |
| 2. ICMPv6 的扩展与变化 | [notes/section-2-ICMPv6的扩展与变化.md](./notes/section-2-ICMPv6的扩展与变化.md) |
| 3. ICMP 套接字 (Ping Sockets) | [notes/section-3-ICMP套接字-Ping-Sockets.md](./notes/section-3-ICMP套接字-Ping-Sockets.md) |
| 4. Iptables 与 ICMP 消息的生成 | [notes/section-4-Iptables与ICMP消息的生成.md](./notes/section-4-Iptables与ICMP消息的生成.md) |

---

## 相关章节

- 上一章：[../chapter-02-netlink-sockets/](../chapter-02-netlink-sockets/)
- 下一章：[../chapter-04-ipv4/](../chapter-04-ipv4/)
- IPv6 / ND：[../chapter-08-IPv6.md](../chapter-08-IPv6.md) · 邻居：[../chapter-07-邻居子系统.md](../chapter-07-邻居子系统.md)
- 组播 MLD：[../note-组播IGMP.md](../note-组播IGMP.md)
- Netfilter：[../chapter-09-Netfilter.md](../chapter-09-Netfilter.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
