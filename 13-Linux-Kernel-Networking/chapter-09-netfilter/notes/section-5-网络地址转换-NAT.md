# Ch 9 §5 网络地址转换 · NAT

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. 网络地址转换 (NAT)

**NAT** 修改 **IP/端口** — 依赖 **conntrack** 记录 **original/reply** 映射，使 **回程包** 可 **逆转换**。

---

## SNAT vs DNAT

| 类型 | 改什么 | 典型 hook | 场景 |
|------|--------|-----------|------|
| **SNAT / MASQUERADE** | **源** IP（+端口） | **POST_ROUTING** | 内网 **出网** 用公网 IP |
| **DNAT / REDIRECT** | **目的** IP（+端口） | **PRE_ROUTING**、LOCAL_OUT | **端口转发**、负载均衡入口 |

```bash
# SNAT
iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 203.0.113.1

# DNAT
iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination 10.0.0.5:8080
```

**`nat_table`：** 在 **除 FORWARD 外的四个 hook** 注册（PREROUTING、POSTROUTING、LOCAL_IN、LOCAL_OUT — 视 target）。

---

## Conntrack 与 NAT 优先级

**同一 hook（如 PRE_ROUTING）上：**

```
priority 更小（更早）→ nf_conntrack_in()   /* 先建/查 nf_conn */
priority 更大（更晚）→ nf_nat_in()         /* 按 ct 状态做 DNAT */
```

**原因：** NAT 必须知道 **是否已有连接**、**reply tuple 如何逆映射** — **必须先 conntrack**。

| 顺序错 | 后果 |
|--------|------|
| NAT 先于 ct | **状态不一致**、回程 **break** |

---

## 与连接状态

- **NEW** 包：NAT 分配 **新映射**，写入 **nf_nat extension**
- **ESTABLISHED**：按 **已有映射** 做 **SNAT/DNAT 逆/正** 变换

**Full cone / symmetric** 等行为由 **tuple 如何改写** 决定 — 影响 **P2P、SIP**。

---

## HFT

| 场景 | 说明 |
|------|------|
| **共置公网 IP** | 常 **无 NAT** — 省 **ct+nat CPU** |
| **容器/K8s** | **大量 SNAT/conntrack** — 与 **低延迟** 冲突，用 **hostNetwork/macvlan** 等 |
| **排查** | `conntrack -L`、`iptables -t nat -vnL` |

---

← [4. iptables](./section-4-IPTables的内核实现.md) · [Ch 9](../README.md) · 下一节 [6. ct 扩展](./section-6-连接跟踪扩展.md)
