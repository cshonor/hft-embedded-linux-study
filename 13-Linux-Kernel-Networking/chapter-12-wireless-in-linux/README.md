# Ch 12 Linux 中的无线网络 · Wireless in Linux

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

> **mac80211** — IEEE **802.11** MAC 层、**CSMA/CA**、**MLME**、Rx/Tx。共置 HFT **几乎全用有线以太网**；本章 **排障 Wi‑Fi 管理口** 或理解 **nl80211** 时 **选读**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | mac80211 · 802.11 头 · BSS/IBSS · 省电 · MLME · 802.11n · Mesh |
| **前置** | [Ch 2 §4 通用 Netlink](../chapter-02-netlink-sockets/notes/section-4-通用Netlink协议.md)（**nl80211**） |
| **HFT 读法** | **⚪ 跳过**；交易机 **禁用 Wi‑Fi / 省电** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. Mac80211 与 802.11 MAC 头部 | [notes/section-1-Mac80211与80211-MAC头部.md](./notes/section-1-Mac80211与80211-MAC头部.md) |
| 2. 网络拓扑结构 | [notes/section-2-网络拓扑结构.md](./notes/section-2-网络拓扑结构.md) |
| 3. 省电模式 | [notes/section-3-省电模式.md](./notes/section-3-省电模式.md) |
| 4. 管理层 MLME | [notes/section-4-管理层MLME.md](./notes/section-4-管理层MLME.md) |
| 5. Mac80211 内核实现 | [notes/section-5-Mac80211内核实现.md](./notes/section-5-Mac80211内核实现.md) |
| 6. 高吞吐量 802.11n | [notes/section-6-高吞吐量80211n.md](./notes/section-6-高吞吐量80211n.md) |
| 7. Mesh 网络 802.11s | [notes/section-7-Mesh网络80211s.md](./notes/section-7-Mesh网络80211s.md) |
| 8. 无线开发流程 | [notes/section-8-无线开发流程.md](./notes/section-8-无线开发流程.md) |

---

## 相关章节

- 上一章：[../chapter-11-layer-4-protocols/](../chapter-11-layer-4-protocols/)
- 下一章：[../chapter-13-InfiniBand.md](../chapter-13-InfiniBand.md)
- Netlink genl：[../chapter-02-netlink-sockets/notes/section-4-通用Netlink协议.md](../chapter-02-netlink-sockets/notes/section-4-通用Netlink协议.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
