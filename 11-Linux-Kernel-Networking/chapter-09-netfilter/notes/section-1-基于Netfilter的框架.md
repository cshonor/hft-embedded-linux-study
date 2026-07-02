# Ch 9 §1 基于 Netfilter 的框架 · Netfilter Frameworks

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 1. 基于 Netfilter 的框架 (Netfilter Frameworks)

**Netfilter**（Rusty Russell, 1998）是 Linux **包过滤/NAT/修改** 的 **内核钩子基础设施** — 上层 **多种框架** 注册在同一 hook 点上。

---

## 常见上层框架

| 框架 | 作用 |
|------|------|
| **iptables / ip6tables** | 经典 **表/链/target** 防火墙与 NAT |
| **nftables** | **iptables 继任** — 统一语法、内核 `nf_tables` 子系统 |
| **ebtables / arptables** | **桥接层 / ARP** 过滤 |
| **IP sets** | **IP/端口/网段集合** — 供 iptables/nft **`-m set`** 引用 |
| **IPVS** | **L4 负载均衡**（VS/NAT/DR/Tunnel）— kube-proxy 等 |

```
                    ┌─────────────┐
  skb ──→ NF_HOOK ──┤ iptables    │
                    │ nftables    │
                    │ conntrack   │
                    │ NAT         │
                    │ IPVS        │
                    └─────────────┘
```

---

## 协议族划分

| 工具 | 协议层 |
|------|--------|
| **iptables** | **IPv4** |
| **ip6tables** | **IPv6** |
| **ebtables** | **以太网桥** |
| **nft inet/inet6** | **统一处理 v4/v6**（现代推荐） |

**用户态：** `iptables`/`nft` → **netlink nfnetlink** — [Ch 2 `NETLINK_NETFILTER`](../chapter-02-netlink-sockets/notes/section-1-Netlink基础与优势.md)

---

## nftables 简述

- **单一规则集**、**增量更新**（相对 iptables 全表替换）
- **内建 set/map** — 替代大量 ipset+iptables 规则
- 生产迁移：**iptables-nft** 兼容层常见

**HFT：** 共置机 **规则数 ≈ 越少越好** — 每条 hook 回调 **增 CPU**；极致路径 **无 iptables** 或 **XDP early drop**（[14-BPF](../../14-BPF-Performance-Tools/chapter-10-networking/)）。

---

← [Ch 9](../README.md) · 下一节 [2. 钩子](./section-2-Netfilter钩子.md)
