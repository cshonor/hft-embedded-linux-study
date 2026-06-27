# Ch 4 IPv4

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> 本章深入 **IPv4 网络层**：头部字段、**Rx/Tx 路径**、**多播**、IP 选项、**分片/重组** 与 **转发**。Ch 5 专讲 **路由子系统**；本章侧重 **包在 IPv4 层的处理逻辑**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | `iphdr` · `ip_rcv` → 路由 → local/forward · 多播 MFC · 分片/defrag · Tx API |
| **前置** | [Ch 1](../chapter-01-introduction/) · [Ch 3](../chapter-03-icmp/) 差错/PMTU |
| **HFT 读法** | **PMTU/DF**、**转发 vs 本地**、**组播行情** 路径；热路径细节见 Ch 11/14 |

### HFT 延伸

| | 笔记 |
|---|------|
| 组播 / IGMP | [note-组播IGMP.md](../note-组播IGMP.md) |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. IPv4 头部 | [notes/section-1-IPv4头部.md](./notes/section-1-IPv4头部.md) |
| 2. 协议初始化与接收路径 | [notes/section-2-协议初始化与接收路径.md](./notes/section-2-协议初始化与接收路径.md) |
| 3. 接收 IPv4 多播数据包 | [notes/section-3-接收IPv4多播数据包.md](./notes/section-3-接收IPv4多播数据包.md) |
| 4. IP 选项 | [notes/section-4-IP选项.md](./notes/section-4-IP选项.md) |
| 5. 发送 IPv4 数据包 | [notes/section-5-发送IPv4数据包.md](./notes/section-5-发送IPv4数据包.md) |
| 6. 分片与重组 | [notes/section-6-分片与重组.md](./notes/section-6-分片与重组.md) |
| 7. 数据包转发 | [notes/section-7-数据包转发.md](./notes/section-7-数据包转发.md) |

---

## 相关章节

- 上一章：[../chapter-03-icmp/](../chapter-03-icmp/)
- 下一章：[../chapter-05-IPv4路由子系统.md](../chapter-05-IPv4路由子系统.md)
- 组播延伸：[../note-组播IGMP.md](../note-组播IGMP.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
