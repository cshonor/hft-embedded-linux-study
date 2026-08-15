# Ch 8 IPv6

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

> IPv6 — **128 位地址**、固定 **40B 头**、**扩展头**、**SLAAC**、**ND/MLD over ICMPv6**。共置 HFT 仍以 **IPv4 为主**；双栈/运营商 **IPv6 行情** 时本章 + [Ch 7 ND](../chapter-07-neighbouring-subsystem/) 选读。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | 地址类型 · 头部 · 扩展头 · SLAAC · Rx/Tx · MLD · `flowi6`/`rt6_info` |
| **前置** | [Ch 4 IPv4](../chapter-04-ipv4/) · [Ch 7 NDISC](../chapter-07-neighbouring-subsystem/notes/section-4-IPv6-NDISC邻居发现.md) |
| **HFT 读法** | 纯 IPv4 机房可 **跳过**；懂 **无广播→组播**、**源端分片** 即可 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. IPv6 地址架构 | [notes/section-1-IPv6地址架构.md](./notes/section-1-IPv6地址架构.md) |
| 2. IPv6 头部 | [notes/section-2-IPv6头部.md](./notes/section-2-IPv6头部.md) |
| 3. 扩展头部 | [notes/section-3-扩展头部.md](./notes/section-3-扩展头部.md) |
| 4. 地址自动配置 | [notes/section-4-地址自动配置-SLAAC.md](./notes/section-4-地址自动配置-SLAAC.md) |
| 5. 接收与转发 | [notes/section-5-数据包的接收与转发.md](./notes/section-5-数据包的接收与转发.md) |
| 6. MLD | [notes/section-6-多播侦听者发现-MLD.md](./notes/section-6-多播侦听者发现-MLD.md) |
| 7. 发送与路由 | [notes/section-7-IPv6的发送与路由机制.md](./notes/section-7-IPv6的发送与路由机制.md) |

---

## 相关章节

- 上一章：[../chapter-07-neighbouring-subsystem/](../chapter-07-neighbouring-subsystem/)
- 下一章：[../chapter-09-netfilter/](../chapter-09-netfilter/)
- ICMPv6 / ND：[../chapter-03-icmp/](../chapter-03-icmp/)
- 组播对照：[../note-组播IGMP.md](../note-组播IGMP.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
