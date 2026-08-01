# Ch 9 §3 连接跟踪 · Connection Tracking

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 3. 连接跟踪 (Connection Tracking)

**Conntrack** 在内核维护 **会话状态**（NEW/ESTABLISHED/RELATED…）— **NAT 的前提**，也可 **单独** 做 **有状态防火墙**。

模块：`nf_conntrack` · 用户态 **`conntrack`/`/proc/net/nf_conntrack`**

---

## 核心数据结构

### `nf_conntrack_tuple` — 五元组（单向）

| 字段 | 说明 |
|------|------|
| **src/dst IP** | L3 |
| **src/dst port**（或 ICMP id 等） | L4 |
| **protocol** | TCP/UDP/ICMP… |

**连接 = 两个方向的 tuple**（original + reply）。

### `nf_conn` — 连接项

| 内容 | 说明 |
|------|------|
| **status** | `IPS_CONFIRMED`、`IPS_NAT_*` 等 |
| **timeout** | 各协议 **gc 超时** |
| **extensions** | NAT、timestamp 等（§6） |
| **master / expectations** | 见下 |

---

## Helpers 与 Expectations

**复杂协议**（**FTP、SIP、H.323**）— 控制连接 **协商** 数据连接端口：

```
FTP 控制 21/tcp
  → ALG/helper 解析 PORT/PASV
  → 创建 **expectation**：「即将有 RELATED 数据连接到 X:Y」
  → 数据连接到达 → 标记 **RELATED** → filter **允许**
```

| 概念 | 作用 |
|------|------|
| **Connection Tracking Helper** | 解析 **应用层** 信令 |
| **Expectation** | **预授权** 尚未出现的 **子连接** |
| **RELATED 状态** | 与 **ESTABLISHED** 主连接 **关联** 的派生流 |

**HFT：** 共置 **纯 TCP/UDP 行情** 通常 **无 ALG**；误开 **nf_conntrack_ftp** 等 **增延迟**。

---

## 状态与 filter

```bash
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -m conntrack --ctstate NEW -p tcp --dport 443 -j ACCEPT
```

**表大小：** `nf_conntrack_max` — 耗尽后 **新连接失败**。

---

← [2. 钩子](./section-2-Netfilter钩子.md) · [Ch 9](../README.md) · 下一节 [4. iptables 实现](./section-4-IPTables的内核实现.md)
