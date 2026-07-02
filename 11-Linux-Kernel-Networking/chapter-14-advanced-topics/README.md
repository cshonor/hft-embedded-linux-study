# Ch 14 高级主题 · Advanced Topics

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

> **高级/前沿子系统** — **netns**、**cgroup tc**、**SO_BUSY_POLL**、通知链；并含蓝牙/802.15.4/NFC 等 **HFT 可跳过** 专题。与 [Ch 1 §2 NAPI](../chapter-01-introduction/notes/section-2-网络设备-net_device.md) 联读 **收包延迟全链路**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | netns · cgroups · Busy Poll · 蓝牙 · 6LoWPAN · NFC · notifier · 杂项 |
| **HFT 精读** | **§1–§3、§7**；§4–§6 **⚪ 跳过** |
| **对照** | [Ch 11 L4](../chapter-11-layer-4-protocols/) · [DPDK 旁路](../../12-DPDK-Low-Latency-Network/) |

---

## 小节笔记

| 节 | 笔记 | HFT |
|----|------|-----|
| 1. 网络命名空间 | [notes/section-1-网络命名空间.md](./notes/section-1-网络命名空间.md) | 🟡 |
| 2. 控制组 Cgroups | [notes/section-2-控制组Cgroups.md](./notes/section-2-控制组Cgroups.md) | 🟡 |
| 3. 忙轮询与收包扩展 | [notes/section-3-忙轮询套接字与收包路径.md](./notes/section-3-忙轮询套接字与收包路径.md) | 🔴 |
| 4. Linux 蓝牙 | [notes/section-4-Linux蓝牙子系统.md](./notes/section-4-Linux蓝牙子系统.md) | ⚪ |
| 5. 802.15.4 与 6LoWPAN | [notes/section-5-IEEE802154与6LoWPAN.md](./notes/section-5-IEEE802154与6LoWPAN.md) | ⚪ |
| 6. 近场通信 NFC | [notes/section-6-近场通信NFC.md](./notes/section-6-近场通信NFC.md) | ⚪ |
| 7. 通知链 | [notes/section-7-通知链.md](./notes/section-7-通知链.md) | 🟡 |
| 8. 其他杂项 | [notes/section-8-其他杂项与补充协议.md](./notes/section-8-其他杂项与补充协议.md) | 🟡 |

---

## 相关章节

- 上一章：[../chapter-13-infiniband/](../chapter-13-infiniband/)
- 下一章：[../appendix-A-Linux-API.md](../appendix-A-Linux-API.md)
- NAPI 入门：[../chapter-01-introduction/notes/section-2-网络设备-net_device.md](../chapter-01-introduction/notes/section-2-网络设备-net_device.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
