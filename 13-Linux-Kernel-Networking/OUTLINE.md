# Rosen — 全书目录（14 章 + 附录 A–C）

> **Linux Kernel Networking: Implementation and Theory** · Rami Rosen

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## 核心章节

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01](./chapter-01-introduction/) | 🟡 |
| 2 | Netlink Sockets | [chapter-02](./chapter-02-Netlink套接字.md) | ⚪ |
| 3 | ICMP | [chapter-03](./chapter-03-ICMP.md) | 🟡 |
| 4 | IPv4 | [chapter-04](./chapter-04-IPv4.md) | 🟡 |
| 5 | The IPv4 Routing Subsystem | [chapter-05](./chapter-05-IPv4路由子系统.md) | 🟡 |
| 6 | Advanced Routing | [chapter-06](./chapter-06-高级路由.md) | 🟡 |
| 7 | Linux Neighbouring Subsystem | [chapter-07](./chapter-07-邻居子系统.md) | 🟡 |
| 8 | IPv6 | [chapter-08](./chapter-08-IPv6.md) | ⚪ |
| 9 | Netfilter | [chapter-09](./chapter-09-Netfilter.md) | ⚪ |
| 10 | IPsec | [chapter-10](./chapter-10-IPsec.md) | ⚪ |
| 11 | Layer 4 Protocols | [chapter-11](./chapter-11-第4层协议.md) | 🔴 |
| 12 | Wireless in Linux | [chapter-12](./chapter-12-无线网络.md) | ⚪ |
| 13 | InfiniBand | [chapter-13](./chapter-13-InfiniBand.md) | 🟡 |
| 16 | Advanced Topics | [chapter-14](./chapter-14-高级主题.md) | 🔴 |

### HFT 延伸

| 主题 | 笔记 | HFT |
|------|------|-----|
| 组播 / IGMP | [note-组播IGMP](./note-组播IGMP.md) | 🔴 |

## 附录

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | Linux API | [appendix-A](./appendix-A-Linux-API.md) | 🟡 |
| B | Network Administration | [appendix-B](./appendix-B-网络管理.md) | 🟡 |
| C | Glossary | [appendix-C](./appendix-C-词汇表.md) | ⚪ |

---

## HFT 精读顺序

```
Ch 11  传输层：Socket / sk_buff / TCP / UDP
Ch 14  高级主题：NAPI / softirq / RSS / RPS / XPS
note   组播 IGMP（行情内核路径）
Ch 4–5 IPv4 / 路由（托管网络时补读）
Ch 13  InfiniBand（共置/RDMA 场景）
```

→ 协议层 → [04-TCP-IP](../12-TCP-IP-Illustrated-Vol1/) · API → [08-UNP](../11-UNP-Vol1/) · 旁路 → [10-DPDK](../14-DPDK-Low-Latency-Network/)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
