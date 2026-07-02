# Linux Kernel Networking — Rami Rosen

**文件夹 06** · 全书 **14 章 + 附录 A–C** · [返回总清单](../READING-LIST.md#4-linux-kernel-networking--rami-rosen)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 核心章节（14 章）

| 章 | 笔记 |
|----|------|
| 1 简介 | [chapter-01-introduction/](./chapter-01-introduction/) |
| 2 Netlink 套接字 | [chapter-02-netlink-sockets/](./chapter-02-netlink-sockets/) |
| 3 ICMP | [chapter-03-icmp/](./chapter-03-icmp/) |
| 4 IPv4 | [chapter-04-ipv4/](./chapter-04-ipv4/) |
| 5 IPv4 路由子系统 | [chapter-05-ipv4-routing-subsystem/](./chapter-05-ipv4-routing-subsystem/) |
| 6 高级路由 | [chapter-06-advanced-routing/](./chapter-06-advanced-routing/) |
| 7 邻居子系统 | [chapter-07-neighbouring-subsystem/](./chapter-07-neighbouring-subsystem/) |
| 8 IPv6 | [chapter-08-ipv6/](./chapter-08-ipv6/) |
| 9 Netfilter | [chapter-09-netfilter/](./chapter-09-netfilter/) |
| 10 IPsec | [chapter-10-ipsec/](./chapter-10-ipsec/) |
| 11 第 4 层协议 | [chapter-11-layer-4-protocols/](./chapter-11-layer-4-protocols/) |
| 12 无线网络 | [chapter-12-wireless-in-linux/](./chapter-12-wireless-in-linux/) |
| 13 InfiniBand | [chapter-13-infiniband/](./chapter-13-infiniband/) |
| 14 高级主题 | [chapter-14-advanced-topics/](./chapter-14-advanced-topics/) |

### HFT 延伸

| | 笔记 |
|---|------|
| 组播 / IGMP | [note-组播IGMP.md](./note-组播IGMP.md) |

## 附录

| | 笔记 |
|---|------|
| A Linux API | [appendix-A-Linux-API.md](./appendix-A-Linux-API.md) |
| B 网络管理 | [appendix-B-网络管理.md](./appendix-B-网络管理.md) |
| C 词汇表 | [appendix-C-词汇表.md](./appendix-C-词汇表.md) |

---

## HFT 精读捷径

```
Ch 11 → Ch 14 → note-组播IGMP
```

**HFT 产出：** 网卡 DMA → NAPI → sk_buff → socket → 用户态；与 UNP、DPDK 对照。Ch 1 建立 **L2–L4 全景** → [chapter-01 §1–§5](./chapter-01-introduction/)。

## 交叉阅读

- 协议层 → [10-TCP-IP-Illustrated-Vol1](../10-TCP-IP-Illustrated-Vol1/)
- API 层 → [09-UNP-Vol1](../09-UNP-Vol1/)
- 用户态旁路 → [12-DPDK-Low-Latency-Network](../12-DPDK-Low-Latency-Network/)
- 跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
