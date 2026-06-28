# Ch 5 IPv4 路由子系统 · The IPv4 Routing Subsystem

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

> 本章讲解 Linux **L3 路由子系统**：**FIB（Forwarding Information Base）** 查表、核心数据结构、**策略路由**、ICMP 重定向，以及 **3.6 移除路由缓存 → FIB TRIE** 的演进。代码基准 **Linux 3.9**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | 转发与 FIB · `fib_lookup` · fib_table/info/alias/nh · 异常 · policy routing · redirect · TRIE |
| **前置** | [Ch 4](../chapter-04-ipv4/) Rx/Tx 与 `ip_forward` |
| **HFT 读法** | 共置机 **静态路由 + 最小 FIB**；理解 **dst 缓存** 与 **PMTU/redirect** 对路径的影响 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 转发与 FIB | [notes/section-1-转发与FIB.md](./notes/section-1-转发与FIB.md) |
| 2. 路由子系统查找 | [notes/section-2-路由子系统查找.md](./notes/section-2-路由子系统查找.md) |
| 3. 核心数据结构 | [notes/section-3-核心数据结构-FIB表与Info与Alias.md](./notes/section-3-核心数据结构-FIB表与Info与Alias.md) |
| 4. FIB 下一跳异常 | [notes/section-4-FIB下一跳异常.md](./notes/section-4-FIB下一跳异常.md) |
| 5. 策略路由 | [notes/section-5-策略路由.md](./notes/section-5-策略路由.md) |
| 6. ICMPv4 重定向 | [notes/section-6-ICMPv4重定向消息.md](./notes/section-6-ICMPv4重定向消息.md) |
| 7. IPv4 路由缓存的移除 | [notes/section-7-IPv4路由缓存的移除与FIB-TRIE.md](./notes/section-7-IPv4路由缓存的移除与FIB-TRIE.md) |

---

## 相关章节

- 上一章：[../chapter-04-ipv4/](../chapter-04-ipv4/)
- 下一章：[../chapter-06-高级路由.md](../chapter-06-高级路由.md)
- ICMP / PMTU：[../chapter-03-icmp/](../chapter-03-icmp/)
- 邻居解析：[../chapter-07-邻居子系统.md](../chapter-07-邻居子系统.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
