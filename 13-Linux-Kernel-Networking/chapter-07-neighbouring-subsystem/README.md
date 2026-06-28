# Ch 7 邻居子系统 · Linux Neighbouring Subsystem

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> 邻居子系统负责 **L3 地址 → L2 硬件地址** 的解析与缓存：**IPv4 ARP**、**IPv6 NDISC**。发包前 **缺 MAC** 时会 **阻塞/排队** — 共置 **静态邻居** 与 **NUD 状态** 影响首包与故障行为。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | `neighbour`/`neigh_table` · ARP · ND · DAD · NUD · `ip neigh` |
| **前置** | [Ch 4 §5 发送路径](../chapter-04-ipv4/notes/section-5-发送IPv4数据包.md) · [Ch 3 §2 ICMPv6/ND](../chapter-03-icmp/notes/section-2-ICMPv6的扩展与变化.md) |
| **HFT 读法** | **静态 ARP/ND**、**NUD FAILED** 排查；与 **ECMP 多 GW**（Ch 6）联动 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 邻居子系统基础与核心作用 | [notes/section-1-邻居子系统基础与核心作用.md](./notes/section-1-邻居子系统基础与核心作用.md) |
| 2. 核心数据结构 | [notes/section-2-核心数据结构-neighbour与neigh_table.md](./notes/section-2-核心数据结构-neighbour与neigh_table.md) |
| 3. IPv4 ARP 协议实现 | [notes/section-3-IPv4-ARP协议实现.md](./notes/section-3-IPv4-ARP协议实现.md) |
| 4. IPv6 NDISC | [notes/section-4-IPv6-NDISC邻居发现.md](./notes/section-4-IPv6-NDISC邻居发现.md) |
| 5. 重复地址检测 DAD | [notes/section-5-重复地址检测-DAD.md](./notes/section-5-重复地址检测-DAD.md) |
| 6. NUD 状态机 | [notes/section-6-NUD网络不可达检测状态机.md](./notes/section-6-NUD网络不可达检测状态机.md) |
| 7. 用户空间交互 | [notes/section-7-用户空间交互.md](./notes/section-7-用户空间交互.md) |

---

## 相关章节

- 上一章：[../chapter-06-advanced-routing/](../chapter-06-advanced-routing/)
- 下一章：[../chapter-08-IPv6.md](../chapter-08-IPv6.md)
- ICMPv6 背景：[../chapter-03-icmp/](../chapter-03-icmp/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
