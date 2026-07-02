# Ch 6 §3 多路径路由 · Multipath Routing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. 多路径路由 (Multipath Routing)

**多路径路由** 为 **同一条路由前缀** 配置 **多个 nexthop** — 内核在 **Rx/Tx** 路径用 **`fib_select_multipath()`** 按 **权重** 选一路，实现 **负载分担 (ECMP)** 或 **冗余**。

---

## 用户态配置

```bash
# 两个下一跳，权重 3 : 5 → 约 37.5% / 62.5% 流量
ip route add 10.0.0.0/8 \
  nexthop via 192.168.1.1 dev eth0 weight 3 \
  nexthop via 192.168.2.1 dev eth1 weight 5
```

| 参数 | 含义 |
|------|------|
| **`nexthop via … dev …`** | 每个 **fib_nh** 一条 |
| **`weight`** | 相对 **流量比例** — 非严格 QoS，是 **统计分流** |
| **同前缀 ECMP** | 单 **fib_info** 挂 **nh[0..n-1]** |

**HFT：** 双上联 **冗余** 常见；**低延迟** 场景需知 **流可能被 hash 到不同路径** — **延迟不一致** 时查 **是否 ECMP**。

---

## 内核结构

```
fib_info
  ├─ fib_nh[0]: via GW1 dev eth0  weight 3
  ├─ fib_nh[1]: via GW2 dev eth1  weight 5
  └─ fib_nh_ext / nexthop group（现代树更泛化）
```

[Ch 5 §3](../../chapter-05-ipv4-routing-subsystem/notes/section-3-核心数据结构-FIB表与Info与Alias.md) 已述 **fib_nh** — 多路径 = **同一 fib_info 多 nh**。

---

## `fib_select_multipath()`

```
fib_lookup 命中 multipath fib_info
  → fib_select_multipath(fib_info, fl4, …)
       ├─ 按 weight 构建 **加权范围**
       ├─ 使用 **hash(flow) + jiffies** 等 **伪随机/流哈希**
       └─ 返回 **选中的 nh index**
  → 后续 forward/xmit 用 **该 nh 的 oif/gw**
```

| 设计点 | 说明 |
|--------|------|
| **Per-flow 一致性** | 同 **5-tuple** 尽量 **固定 nh** — 避免 **乱序**（具体 hash 键随内核版本） |
| **jiffies** | 引入 **随时间变化** — 长期 **重平衡** |
| **Rx vs Tx** | **转发与本地发出** 均可触发 **选路** |

**现代演进：** **nexthop 对象**、**IPv6 MP hash**、**BPF** 等 — 3.9 后 **ECMP 更精细**，原理不变。

---

## 与策略路由组合

```
ip rule  → 选 **哪张表**
ip route → 表内 **ECMP 前缀**
```

例：交易表 **default** 两 nexthop — **仅交易流量**（`from` rule）走 ECMP。

---

## HFT checklist

- [ ] `ip route show` 是否 **nexthop 多行** / `proto boot` ECMP
- [ ] **延迟抖动** 是否因 **跨链路 ECMP** — 可 **绑源** 或 **reduce to single nh**
- [ ] **ARP/邻居** 每条 nh 独立 — [Ch 7](../../chapter-07-neighbouring-subsystem/)
- [ ] DPDK **bonding** 在用户态 — **不等价** 内核 ECMP

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **mrouted + MFC + vif**；主机 join ≠ mr 路由器 |
| §2 | **`fib_rules`、local/main/default、`ip rule`** |
| §3 | **ECMP weight、`fib_select_multipath`** |

---

## 相关章节

- 下一章：[Ch 7 邻居子系统](../../chapter-07-neighbouring-subsystem/)
- FIB 基础：[Ch 5](../../chapter-05-ipv4-routing-subsystem/)

---

← [2. 策略路由](./section-2-策略路由.md) · [Ch 6](../README.md)
