# TCP/IP Illustrated, Vol.1 — 2nd Edition (Fall, 2016)

> **《TCP/IP 详解 卷1：协议》第 2 版** · Kevin R. Fall 等 · **全书 18 章**。  
> 不是 Stevens 1994 的 30 章老版；**无 Telnet/FTP/SMTP/SNMP 独立章**。

| 文档 | 用途 |
|------|------|
| [VERSIONS.md](./VERSIONS.md) | 第 1 版 30 章 vs 第 2 版 18 章 |
| [OUTLINE.md](./OUTLINE.md) | 18 章目录与文件夹映射 |
| [QUICKREF.md](./QUICKREF.md) | **每章考点 + Go/Rust 一页速览** |

## 章节目录（`chapterXX-主题/study.md` + 平铺 `1.x-*.md`）

各节笔记在**章文件夹根目录**（如 `1.1-architecture-principles.md`），与 `study.md` 同级，打开一章即可扫全节，无需再点进子文件夹。

| 章 | 文件夹 | 笔记 |
|----|--------|------|
| 1–2 | `chapter01-overview` … `chapter02-ip-address-architecture` | 体系架构 |
| 3 | `chapter03-link-layer` | 链路层 |
| 4–8 | `chapter04-arp-protocol` … `chapter08-icmpv4-icmpv6` | 网络层 |
| 9–10, 12–17 | `chapter09-broadcast-multicast` … `chapter17-tcp-keepalive` | 传输层 |
| 11, 18 | `chapter11-dns-domain-resolve` · `chapter18-network-security` | 应用与安全 |

完整映射见 [OUTLINE.md](./OUTLINE.md)。

## 与源笔记目录

精读正文由 [`tcpip_vol1_ed2_notes/`](../tcpip_vol1_ed2_notes/) 同步至本仓库各章 `study.md`；后续可在任一侧编辑后再次同步。

与 **[自顶向下](../README.md)** 并行；TCP/UDP 精读见 [03_transport_layer/study.md](../top_down/03_transport_layer/study.md)。
