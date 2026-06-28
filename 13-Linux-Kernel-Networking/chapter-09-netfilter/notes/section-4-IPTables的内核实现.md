# Ch 9 §4 IPTables 内核实现 · IPTables Kernel Implementation

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. IPTables 的内核实现

**iptables** 在内核由 **`x_tables`** 框架实现 — 每张 **表** 一个 **`xt_table`**，每条 **规则** 为 **`ipt_entry` + matches + target**。

---

## 表与链（IPv4 经典）

| 表 | 内置链 | 挂载 hook |
|----|--------|-----------|
| **raw** | PREROUTING, OUTPUT | PRE_ROUTING, LOCAL_OUT（**最早**，可 NOTRACK） |
| **mangle** | 全部五链 | 改 TOS/mark/TTL |
| **nat** | PREROUTING, POSTROUTING, OUTPUT, … | DNAT/SNAT |
| **filter** | INPUT, FORWARD, OUTPUT | ACCEPT/DROP/REJECT |

**遍历顺序：** 某 hook 上 **按表 priority** → **链内规则自上而下** — **首 match 的 target 决定**（ACCEPT 可仍继续视配置）。

---

## 内核结构

| 结构 | 角色 |
|------|------|
| **`xt_table`** | 表名 `filter`/`nat`、valid_hooks、entries |
| **`ipt_entry`** | 规则头 + **`ipt_ip`** 匹配条件 |
| **`xt_match`** | `-m tcp`、`-m conntrack` 等 |
| **`xt_target`** | `-j ACCEPT/DROP/LOG/SNAT`… |

**更新路径：** 用户 `iptables -A` → **setsockopt SO_SET_REPLACE** / **nft netlink** → 内核 **原子替换** 规则 blob。

---

## 示例：UDP 5001 → LOG（原书）

```bash
iptables -A INPUT -p udp --dport 5001 -j LOG --log-prefix "udp5001: "
iptables -A INPUT -p udp --dport 5001 -j ACCEPT
```

**包路径（入站本地）：**

```
PRE_ROUTING（raw/mangle/nat）
  → 路由 → LOCAL_IN
       → filter INPUT：match dport 5001
            → LOG target：nf_log → syslog
            → ACCEPT → 继续 ip_local_deliver → UDP socket
```

**转发包** 走 **FORWARD 链**，不经过 INPUT — 规则 **挂错链 = 不生效**。

---

## HFT 注意

| 点 | 说明 |
|----|------|
| **规则顺序** | 首条 match **定生死** |
| **`-j LOG`** | **极耗 CPU** — 勿在 tick 路径端口上开 |
| **计数器** | `-v` 看 **pkts/bytes** 验证是否 hit |
| **nft 等价** | `nft add rule inet filter input udp dport 5001 log` |

→ REJECT/ICMP：[Ch 3 §4](../../chapter-03-icmp/notes/section-4-Iptables与ICMP消息的生成.md)

---

← [3. 连接跟踪](./section-3-连接跟踪.md) · [Ch 9](../README.md) · 下一节 [5. NAT](./section-5-网络地址转换-NAT.md)
