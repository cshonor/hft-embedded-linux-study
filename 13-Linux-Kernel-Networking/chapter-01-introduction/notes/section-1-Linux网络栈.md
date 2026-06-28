# Ch 1 §1 Linux 网络栈 · The Linux Network Stack

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. Linux 网络栈 (The Linux Network Stack)

OSI 模型有 **七层**；**Linux 内核网络栈** 主要实现其中 **三层**——其余由 **硬件** 或 **用户空间** 承担。

---

## OSI 与内核分工

| 层 | OSI | 内核是否处理 | 典型内容 |
|----|-----|--------------|----------|
| L7–L5 | 应用 / 表示 / 会话 | **否** — 用户态 | HTTP、FIX、行情解码、TLS（用户态库） |
| **L4** | 传输层 | **是** | **TCP、UDP** — 端口、连接、重传、拥塞 |
| **L3** | 网络层 | **是** | **IPv4/IPv6** — 寻址、转发、分片 |
| **L2** | 数据链路层 | **是** | **以太网驱动**、`net_device`、VLAN、桥接 |
| L1 | 物理层 | **否** — 硬件 | PHY、SerDes、光纤/铜缆 |

**内核栈的核心任务：**

```
收包：L2 驱动 → L3（本地交付 or 转发）→ 若本地 → L4 → socket 队列 → 用户态 read
发包：用户态 write → L4 构包 → L3 路由选路 → L2 驱动 DMA 发出
转发：L3 查路由 → 不经 L4 → 直接送回 L2 另一接口
```

> **HFT：** 共置机上的 **行情/发单** 若走 **标准内核栈**，延迟热点在 **L2→L3→L4→copy_to_user** 整条链；**DPDK/XDP** 是在 **L2 附近旁路** 内核后半段（→ [14-DPDK](../../14-DPDK-Low-Latency-Network/)）。

---

## 三层协作一图

```
┌─────────────────────────────────────────────────────────┐
│  用户空间：应用程序（L5+）— socket API / epoll / FIX     │
└───────────────────────────┬─────────────────────────────┘
                            │ syscall
┌───────────────────────────▼─────────────────────────────┐
│  L4  net/ipv4/tcp*.c · udp.c  — 端口、连接、队列          │
├───────────────────────────▼─────────────────────────────┤
│  L3  net/ipv4/ip*.c · 路由 FIB · Netfilter hooks         │
├───────────────────────────▼─────────────────────────────┤
│  L2  drivers/net/*.c · net_device · NAPI · qdisc         │
└───────────────────────────┬─────────────────────────────┘
                            ▼
                         网卡硬件
```

---

## 与全书后续章节的关系

| 本章概念 | 后续展开 |
|----------|----------|
| L3 转发/本地交付 | [Ch 5 IPv4 路由](../../chapter-05-ipv4-routing-subsystem/) · [Ch 6 高级路由](../../chapter-06-advanced-routing/) |
| L2 邻居解析 | [Ch 7 邻居子系统](../../chapter-07-neighbouring-subsystem/) |
| Netfilter 钩子 | [Ch 9 Netfilter](../../chapter-09-netfilter/) |
| L4 TCP/UDP | [Ch 11 第 4 层协议](../../chapter-11-layer-4-protocols/) — **HFT 精读** |
| NAPI / softirq / RSS | [Ch 14 高级主题](../../chapter-14-高级主题.md) — **HFT 精读** |

---

← [Ch 1](../README.md) · 下一节 [2. 网络设备 net_device](./section-2-网络设备-net_device.md)
