# Ch 6 高级路由 · Advanced Routing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> 在 [Ch 5](../chapter-05-ipv4-routing-subsystem/) **FIB 基础** 之上，本章展开 **多播路由**、**策略路由实现** 与 **多路径 (ECMP) 路由** — 共置 HFT 中 **组播行情** 与 **多出口/冗余链路** 会触及前两者。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | Multicast + `mrouted`/`pimd` · `fib_rules` · multipath weight |
| **前置** | [Ch 4 §3 多播接收](../chapter-04-ipv4/notes/section-3-接收IPv4多播数据包.md) · [Ch 5 §5 策略路由](../chapter-05-ipv4-routing-subsystem/notes/section-5-策略路由.md) |
| **HFT 读法** | **组播行情** → §1 + [note-组播IGMP](../note-组播IGMP.md)；**双上联 ECMP** → §3 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 多播路由 | [notes/section-1-多播路由.md](./notes/section-1-多播路由.md) |
| 2. 策略路由 | [notes/section-2-策略路由.md](./notes/section-2-策略路由.md) |
| 3. 多路径路由 | [notes/section-3-多路径路由.md](./notes/section-3-多路径路由.md) |

---

## 相关章节

- 上一章：[../chapter-05-ipv4-routing-subsystem/](../chapter-05-ipv4-routing-subsystem/)
- 下一章：[../chapter-07-neighbouring-subsystem/](../chapter-07-neighbouring-subsystem/)
- 组播延伸：[../note-组播IGMP.md](../note-组播IGMP.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
