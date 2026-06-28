# Ch 3 §4 Iptables 与 ICMP 消息的生成

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. Iptables 与 ICMP 消息的生成

防火墙除 **静默丢弃 (DROP)** 外，可用 **`REJECT`** 目标 **主动发 ICMP 差错** — 告知对端 **「被拒绝」及原因**，与栈内 **`icmp_send()`** 自然生成的不可达 **同一族消息**。

现代等价：**nftables `reject`** — 原理相同；原书以 **iptables** 为例。

---

## REJECT vs DROP

| 目标 | 对端观察 | ICMP |
|------|----------|------|
| **DROP** | 超时、重传 | **无** |
| **REJECT** | 立即失败 | **有** — 指定 type/code |

**HFT / 低延迟：**  
- **DROP** — 少 CPU、少泄露拓扑；排查难。  
- **REJECT** — 快速 fail；多 **ICMP 开销**，可能 **暴露过滤规则**。

---

## `--reject-with` 示例

```bash
# 拒绝入站 SSH 并回复「主机被管理禁止」
iptables -A INPUT -p tcp --dport 22 -j REJECT --reject-with icmp-host-prohibited

# 拒绝 UDP 某端口 — 端口不可达（像无服务监听）
iptables -A INPUT -p udp --dport 9999 -j REJECT --reject-with icmp-port-unreachable
```

| `--reject-with` | ICMP 含义 |
|-----------------|-----------|
| `icmp-net-unreachable` | 网络不可达 |
| `icmp-host-unreachable` | 主机不可达 |
| `icmp-port-unreachable` | 端口不可达（UDP 常见） |
| `icmp-host-prohibited` | 管理禁止 |
| `tcp-reset` | **TCP RST**（非 ICMP，仅 TCP） |

内核在 **Netfilter hook** 判定 REJECT 时调用 **`icmp_send()`** 或 TCP **RST** 生成路径 — 与 §1 栈内异常 **共用 ICMP 基础设施**。

---

## 与 Netfilter 栈位置

```
入站包 → PRE_ROUTING → … → filter INPUT
                              ↓ REJECT
                         icmp_send / send_reset
                              ↓
                         从本机接口发出 ICMP
```

→ 深读：[Ch 9 Netfilter](../../chapter-09-netfilter/) · [Ch 1 §4 钩子](../../chapter-01-introduction/notes/section-4-数据包的收发与流转.md)

---

## 运维 checklist

- [ ] 交易 VLAN **默认 DROP** 时，确认 **PMTU 所需 ICMP** 是否误拦（type 3 code 4）
- [ ] **`REJECT --reject-with`** 是否与对端 **超时策略** 一致（避免长时间 SYN 重试）
- [ ] 现代环境对照 **nft** 语法：`reject with icmp type …`

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | ICMPv4 **差错 vs 查询**、`icmp_send`、**限速** |
| §2 | ICMPv6 + **ND/MLD** 取代 ARP/IGMP |
| §3 | **Ping socket**、`ping_group_range` |
| §4 | **iptables REJECT** 触发 ICMP |

---

## 相关章节

- 下一章：[Ch 4 IPv4](../../chapter-04-ipv4/)
- Netfilter：[Ch 9](../../chapter-09-netfilter/)

---

← [3. Ping 套接字](./section-3-ICMP套接字-Ping-Sockets.md) · [Ch 3](../README.md)
