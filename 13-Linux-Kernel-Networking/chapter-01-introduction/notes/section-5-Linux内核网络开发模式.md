# Ch 1 §5 Linux 内核网络开发模式 · The Linux Kernel Networking Development Model

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 5. Linux 内核网络开发模式 (The Linux Kernel Networking Development Model)

本章收尾介绍 **如何参与或跟踪** Linux 网络子系统开发：版本控制、邮件列表、补丁流程。与 Gorman VM 书 Ch 1「读内核方法论」 **同构** — 网络子系统有 **独立的 netdev 文化**。

---

## 版本与代码基准

| 项 | 说明 |
|----|------|
| **本书代码** | 主要基于 **Linux kernel 3.9** |
| **阅读现代树** | 用 [Elixir Bootlin](https://elixir.bootlin.com/linux/latest/source/net/) 对照 **`net/`** 目录 |
| **API 漂移** | 3.9 → 6.x：`struct net_device_ops`、NAPI、GRO、TLS offload、BPF/XDP 等 **增量很大** — **原则不变，函数名常变** |

---

## 开发工作流

```
git clone linux.git
  → 改 drivers/net/ 或 net/ipv4/ ...
  → git format-patch / send-email
  → netdev 邮件列表 review
  → Maintainer 入主线
```

| 组件 | 用途 |
|------|------|
| **`git`** | 版本控制、format-patch、bisect 回归 |
| **netdev 邮件列表** | **网络子系统** 补丁主战场（`netdev@vger.kernel.org`） |
| **LKML / lore.kernel.org** | 归档检索、线程回复 |
| **`scripts/checkpatch.pl`** | 风格检查 |
| **`Documentation/networking/`** | 官方网络文档（含 driver、NAPI、scaling） |

**与 MM 子系统对比：** VM 补丁常走 **linux-mm**；网络走 **netdev** — **订阅对口列表** 比泛 LKML 高效。

---

## 读源码建议路线（配合全书）

| 顺序 | 路径 | 目的 |
|------|------|------|
| 1 | `net/core/dev.c` — `netif_receive_skb` | 收包入口 |
| 2 | `net/ipv4/ip_input.c` — `ip_rcv` | L3  demux |
| 3 | `net/ipv4/tcp_ipv4.c` — `tcp_v4_rcv` | L4 入口（→ Ch 11） |
| 4 | `drivers/net/ethernet/...` 任选主流驱动 | NAPI、ring、xmit |

**HFT 读法：** 不必先贡献补丁；用 **Bootlin + bpftrace/tcpdump** 把 **Ch 1 地图** 和 **真实 skb 轨迹** 对齐即可。

---

## 本章带走的三句话

1. 内核管 **L2–L4**；**L5+ 在用户态** — socket 是边界。
2. **`net_device` + `sk_buff`** 是一切路径的 **两个轴**。
3. **路由 + Netfilter** 决定每个包 **去哪、活不活** — 调优和排障都绕不开。

---

## 相关章节

- 下一章：[Ch 2 Netlink](../../chapter-02-netlink-sockets/) — 用户态配置网络栈的 **rtnetlink** 通道
- HFT 精读：[Ch 11 L4](../../chapter-11-layer-4-protocols/) · [Ch 14 高级主题](../../chapter-14-高级主题.md)
- 旁路对照：[14-DPDK](../../14-DPDK-Low-Latency-Network/)

---

← [4. 收发包流转](./section-4-数据包的收发与流转.md) · [Ch 1](../README.md)
