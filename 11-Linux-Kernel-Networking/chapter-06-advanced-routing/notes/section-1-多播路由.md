# Ch 6 §1 多播路由 · Multicast Routing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. 多播路由 (Multicast Routing)

**多播（组播）** 向 **多个接收者** 发 **同一内容** — 目的 **224.0.0.0/4（D 类）**；流媒体、视频会议、**交易所行情 feed** 等场景 **节省带宽**（相对多次单播复制）。

---

## 与单播的本质差异

| | 单播 | 多播 |
|---|------|------|
| 目的 | 一个 IP | **组地址** — 动态成员集 |
| FIB | `fib_lookup` → 一条 nh | **(S,G) / (*,G)** 树 — **复制到多出口** |
| 内核 alone | **足够**（转发路由器） | **不够** — 需 **用户态 mrouting daemon** |

**Linux 内核无法单独完成多播路由决策** — 必须与 **`mrouted`**、**`pimd`**（PIM）等 **用户空间守护进程** 配合：daemon 跑 **组播路由协议**，通过 **netlink/socket** 把 **MFC 项** 写进内核。

**仅主机收组播（非 mr 路由器）：** IGMP + [Ch 4 §3](../../chapter-04-ipv4/notes/section-3-接收IPv4多播数据包.md) 即可 — **不必** 启 `mrouted`。

→ IGMP 深读：[note-组播IGMP](../../note-组播IGMP.md)

---

## IGMP（组成员管理 · 简述）

**Internet Group Management Protocol** — 主机通知 **本地路由器**：「我要/不要 **某组**」。

| 版本 | 要点 |
|------|------|
| IGMPv2 | Join/Leave、Leave 查询 |
| IGMPv3 | **源过滤 (SSM)** — 指定 **允许源 S** |

内核 **`igmp.c`** 维护 per-interface **组播过滤器**；交换机侧常开 **IGMP Snooping** — HFT 机房 **VLAN 泛洪** 与 **精确复制** 的分界。

---

## 核心数据结构

### `mr_table` — 多播路由表

每个 **网络命名空间** 可有一张 **`mr_table`** — 存 **多播路由规则**（由 daemon 配置），与 **单播 fib_table** 并行。

### MFC — Multicast Forwarding Cache

**多播转发表缓存** — 数据面 **真正查的结构**：

```
mfc_cache 项：(origin S, group G) → 出 vif 列表 + TTL 阈值
```

| 概念 | 作用 |
|------|------|
| **(S,G)** | 特定 **源 S** 到 **组 G** 的转发状态 |
| **(*,G)** | 任意源到组 G（ASM） |
| **cache 命中** | **快速复制** 到多个 tunnel/物理 vif |
| **miss** | 可能 **upcall** 用户 daemon 建项 |

MFC 是 **数组/哈希缓存** — 类似单播 **dst**，但键是 **(S,G)** 而非 **dest prefix**。

### `vif_device` — 虚拟多播接口

**Virtual Interface** — 多播转发 **逻辑出口**：

| 类型 | 说明 |
|------|------|
| 基于 **物理 dev** | 某 eth 作为一个 vif |
| **IPIP 隧道** | 跨域组播 **封装** |

`mrouted`/`pimd` 在 vif 上 **建立组播树** — 内核 **按 MFC 从入 vif 复制到出 vif 集**。

---

## 接收与转发路径 (Rx)

```
ip_route_input_mc()          /* Ch 4 — 是否本地/mr 路由器 */
  → 若配置为 multicast router：
       dst.input = ip_mr_input()

ip_mr_input()
  → MFC lookup (S,G)
       ├─ hit  → ip_mr_forward() — 复制到各 out vif，TTL--
       └─ miss → 可能 userspace upcall / drop
```

| 函数 | 角色 |
|------|------|
| **`ip_route_input_mc()`** | 多播 **输入路由** — 与单播分支不同 |
| **`ip_mr_input()`** | **mr 路由器入口** |
| **`ip_mr_forward()`** | **按 MFC 转发/复制** |

**本地交付：** 若 **本机 join 了 G**，仍走 **local deliver** → UDP（行情 socket）。

---

## HFT 要点

| 主题 | 行动 |
|------|------|
| **共置收行情** | 通常 **IGMP join + UDP**，**非** mr 路由器 — §1 **MFC/mrouted** 为 **跨网组播转发** |
| **feed 进机房** | 可能经 **PIM/mroute** — 与 **交易主机** 分层 |
| **内核路径** | 仍 **L2 → ip_rcv → mc → UDP**；DPDK **用户态组播** 另论 |
| **排查** | `ip mroute show`、`cat /proc/net/ip_mr_*`（视内核版本） |

---

← [Ch 6](../README.md) · 下一节 [2. 策略路由](./section-2-策略路由.md)
