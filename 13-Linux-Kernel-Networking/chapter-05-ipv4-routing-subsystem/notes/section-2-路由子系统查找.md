# Ch 5 §2 路由子系统查找 · Performing a Lookup

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 路由子系统查找 (Performing a Lookup)

**Rx 与 Tx** 路径在需要 **「往哪送」** 时都会调用 **`fib_lookup()`**（3.9 时代 FIB 核心入口之一）— 成功则得到 **`fib_result`**，并构建 **`rtable` / `dst_entry`** 供后续 **input/output** 回调使用。

---

## 查找键：`flowi4`

```c
/* 概念字段 */
struct flowi4 {
    __be32 daddr;      /* 目的 IP */
    __be32 saddr;      /* 源 IP（可选，策略路由/strong end） */
    u8     tos;        /* TOS / DSCP */
    u8     scope;
    u8     protocol;
    int    oif;        /* 出接口 hint（Tx） */
    int    iif;        /* 入接口（Rx） */
    /* … mark、uid 等（策略路由扩展） */
};
```

**一次 lookup** = 用 **flowi4** 在 **选定 fib_table** 中 **最长前缀匹配 (LPM)**。

---

## 查找流程（简化）

```
fib_lookup(net, fl4, res, flags)
  → 选 table（main / local / policy 选表 §5）
  → LPM 匹配前缀 → fib_info + nexthop
  → 填充 fib_result（type、scope、nh_oif、metrics…）
  → 构建/查找 rtable → dst_entry
       ├─ .input  = ip_local_deliver 或 ip_forward …
       └─ .output = ip_output …
```

| 对象 | 作用 |
|------|------|
| **`fib_result`** | 一次 lookup 的 **即时结果**（nh、type、table） |
| **`rtable`** | IPv4 **路由缓存条目** 的载体（3.6 后语义变化 — §7） |
| **`dst_entry`** | **通用目的地缓存** — input/output/邻居/metrics |

**skb 绑定：** `skb_dst_set(skb, &rt->dst)` — 后续 **同一包** 不再全表查。

---

## Rx vs Tx

| 方向 | 典型调用 | 关注点 |
|------|----------|--------|
| **Rx** | `ip_route_input_noref()` | 目的 = 包 **daddr**；决定 local/forward |
| **Tx** | `ip_route_output_key()` | 源/目的/ mark；选 **出接口与下一跳 GW** |

**HFT：** 高频 **固定 5-tuple** 时 **dst 缓存命中** 降 lookup 成本；换 **目的 IP/接口** 则 miss → **`fib_lookup` 慢路径**。

---

## 失败与 ICMP

| 结果 | 行为 |
|------|------|
| **不可达** | drop + **ICMP net/host unreachable**（[Ch 3](../../chapter-03-icmp/)） |
| **需网关 ARP** | 先 **邻居解析**（[Ch 7](../../chapter-07-邻居子系统.md)） |

---

← [1. 转发与 FIB](./section-1-转发与FIB.md) · [Ch 5](../README.md) · 下一节 [3. 数据结构](./section-3-核心数据结构-FIB表与Info与Alias.md)
