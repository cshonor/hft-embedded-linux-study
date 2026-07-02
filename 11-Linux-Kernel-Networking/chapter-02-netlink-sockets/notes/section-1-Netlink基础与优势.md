# Ch 2 §1 Netlink 基础与优势 · Netlink Basics & Advantages

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 1. Netlink 套接字基础与优势 (Netlink Basics & Advantages)

**Netlink** 是一种 **基于套接字的 IPC**，自 **Linux 2.2** 起以 **`AF_NETLINK`** 引入，用来 **替代繁琐的 ioctl** 配置网络与内核对象。

---

## 通信模型

```
用户空间进程  ←—— Netlink 消息 ——→  内核子系统（路由、邻居、Netfilter…）
                    ↑
              内核子系统之间也可互发（如 rtnetlink 事件）
```

| 角色 | 典型用途 |
|------|----------|
| **用户 → 内核** | `ip route add`、`ip link set`、`tc` 下发规则 |
| **内核 → 用户** | 链路 up/down、地址变更、路由更新 **异步通知** |
| **内核 ↔ 内核** | 子系统间传递 rtnetlink 事件（较少直接由应用关心） |

---

## 相对 ioctl 的优势

| | **ioctl**（老 net-tools） | **Netlink**（iproute2） |
|---|--------------------------|-------------------------|
| 用户等待 | 常需 **轮询** 或反复 ioctl 查状态 | **`recvmsg` 阻塞** 等内核推送 |
| 内核主动通知 | 困难 | **内核可异步 unicast/multicast** |
| 多订阅者 | 不自然 | **多播组** — 同一事件通知多个监听进程 |
| 扩展性 | 命令号易冲突、结构体固定 | **TLV 属性** 可扩展（§3） |
| 工具代表 | `ifconfig`、`route`（**net-tools**） | **`ip`、`ss`、`tc`**（**iproute2**） |

> **HFT：** 行情/发单 **热路径不走 Netlink**；但 **`ip link` 设 XPS/RSS、`ip route` 静态路由、`ss` 看 backlog** 都是 Netlink — **上线 checklist** 会用到。

---

## 常见 Netlink 协议族（部分）

| 协议 | 用途 |
|------|------|
| **`NETLINK_ROUTE`** (rtnetlink) | 链路、地址、路由、规则、traffic control |
| **`NETLINK_NETFILTER`** | 与 nfnetlink 配合（iptables/nftables 用户态） |
| **`NETLINK_GENERIC`** | Generic Netlink 多路复用（§4） |
| **`NETLINK_SOCK_DIAG`** | 套接字诊断（§5，`ss` 命令） |

---

← [Ch 2](../README.md) · 下一节 [2. 创建与交互](./section-2-套接字的创建与交互.md)
