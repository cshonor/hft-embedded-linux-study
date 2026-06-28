# Ch 9 Netfilter 框架 · Netfilter

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

> **Netfilter** 在栈内 **5 个 hook** 插入回调 — **iptables/nftables、NAT、conntrack、IPVS** 均建其上。共置 HFT 常 **精简/旁路规则**；懂 hook 顺序与 **conntrack→NAT** 优先级利于 **排障与 mark 策略路由**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | IPVS/ipset/iptables/nft · 5 hooks · conntrack · xt_table · SNAT/DNAT · ct 扩展 |
| **前置** | [Ch 1 §4 钩子](../chapter-01-introduction/notes/section-4-数据包的收发与流转.md) · [Ch 4/8 Rx/Tx](../chapter-04-ipv4/) |
| **HFT 读法** | 热路径 **少规则**；需要时用 **mark + ip rule**（Ch 6）· 旁路见 DPDK/XDP |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 基于 Netfilter 的框架 | [notes/section-1-基于Netfilter的框架.md](./notes/section-1-基于Netfilter的框架.md) |
| 2. Netfilter 钩子 | [notes/section-2-Netfilter钩子.md](./notes/section-2-Netfilter钩子.md) |
| 3. 连接跟踪 | [notes/section-3-连接跟踪.md](./notes/section-3-连接跟踪.md) |
| 4. IPTables 内核实现 | [notes/section-4-IPTables的内核实现.md](./notes/section-4-IPTables的内核实现.md) |
| 5. 网络地址转换 NAT | [notes/section-5-网络地址转换-NAT.md](./notes/section-5-网络地址转换-NAT.md) |
| 6. 连接跟踪扩展 | [notes/section-6-连接跟踪扩展.md](./notes/section-6-连接跟踪扩展.md) |

---

## 相关章节

- 上一章：[../chapter-08-ipv6/](../chapter-08-ipv6/)
- 下一章：[../chapter-10-IPsec.md](../chapter-10-IPsec.md)
- iptables REJECT/ICMP：[../chapter-03-icmp/notes/section-4-Iptables与ICMP消息的生成.md](../chapter-03-icmp/notes/section-4-Iptables与ICMP消息的生成.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
