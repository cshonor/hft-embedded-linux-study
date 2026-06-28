# Ch 7 §2 核心数据结构 · `neighbour` 与 `neigh_table`

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 核心数据结构：`neighbour` 与 `neigh_table`

邻居子系统 **协议无关** 的骨架是 **`struct neighbour`**；每种 L3 协议挂一张 **`neigh_table`** 做 **哈希缓存**。

---

## `struct neighbour`

代表 **一条 L2 链路上的一个邻站**（通常 keyed by **L3 地址 + 设备**）：

| 成员 | 含义 |
|------|------|
| **`primary_key`** | L3 地址（IPv4/IPv6 字节） |
| **`ha`** | **Hardware address** — MAC 等 |
| **`dev`** | 绑定的 **`net_device`** |
| **`nud_state`** | **NUD_*** 可达性状态（§6） |
| **`timer`** | 探测/过期 **定时器** |
| **`arp_queue`** | **待决包队列** — 解析完成前 **暂存 skb** |
| **`ops`** | 协议相关 **arp/ndisc** 回调 |

**一次解析、多次使用：** 命中 **REACHABLE/STALE** 时 **直接填 MAC**，无需广播。

---

## `struct neigh_table`

**邻居表** — 缓存 **L3→L2** 映射，带 **GC（垃圾回收）**：

| 表 | 协议 | 全局实例 |
|----|------|----------|
| ARP | IPv4 | **`arp_tbl`** |
| NDISC | IPv6 | **`nd_tbl`** |

| 阈值 | 典型作用 |
|------|----------|
| **`gc_thresh1`** | 低于此 **停止** 异步 GC |
| **`gc_thresh2`** | 超过则 **启动** 异步 GC |
| **`gc_thresh3`** | 超过则 **同步** 强制回收 — 防 **表爆炸** |

用户可见：`/proc/sys/net/ipv4/neigh/default/gc_thresh*`（IPv6 有对应项）。

---

## 查找与创建

```
neigh_lookup / __neigh_lookup
  → 哈希 (key, dev)
  → hit：更新 used 时间戳
  → miss：neigh_create → NUD_NONE → 触发 solicit
```

**Per-netns：** 容器各自 **邻居表** — 与 **net_device 命名空间** 一致。

---

## 与 `dst_entry` 的关系

路由 **`dst`** 指向 **下一跳 neighbour**（或 **direct neighbour**）— **FIB 选路 + neighbour 选 MAC** 串联。

```
fib_lookup → dst → neigh_connected_output
```

---

← [1. 基础](./section-1-邻居子系统基础与核心作用.md) · [Ch 7](../README.md) · 下一节 [3. ARP](./section-3-IPv4-ARP协议实现.md)
